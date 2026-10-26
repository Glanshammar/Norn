using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.IO.Compression;
using Microsoft.AspNetCore.StaticFiles;
using Norn.API.Context;
using Norn.API.DTO;
using Norn.API.Entities;

namespace Norn.API.Controllers;

[ApiController]
[Route("api/[controller]")]
public class FilesController : ControllerBase
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<FilesController> _logger;
    private readonly FileExtensionContentTypeProvider _contentTypeProvider = new();

    public FilesController(ApplicationDbContext context, ILogger<FilesController> logger, IConfiguration configuration)
    {
        _context = context;
        _logger = logger;
    }

    // Get all/selective files
    [HttpGet]
    [ProducesResponseType(typeof(IEnumerable<Files>), StatusCodes.Status200OK)]
    public async Task<ActionResult<IEnumerable<Files>>> GetFiles(
        [FromQuery] string? search = null,
        [FromQuery] string? fileType = null,
        [FromQuery] DateTime? minCreated = null)
    {
        try
        {
            var query = _context.Files.AsNoTracking();
            
            if (!string.IsNullOrEmpty(search))
            {
                var sanitizedSearch = search.Replace("[", "[[]").Replace("%", "[%]");
                query = query.Where(d => EF.Functions.Like(d.Name, $"%{sanitizedSearch}%") || 
                                         EF.Functions.Like(d.FilePath, $"%{sanitizedSearch}%"));
            }
            
            if (!string.IsNullOrEmpty(fileType))
            {
                var categoryMap = new Dictionary<string, string[]>
                {
                    ["audio"] = [".mp3", ".wav", ".flac", ".aac"],
                    ["image"] = [".jpg", ".png", ".gif", ".webp"],
                    ["video"] = [".mp4", ".avi", ".mkv", ".webm"],
                    ["document"] = [".pdf", ".docx", ".txt", ".md"]
                };

                // Split and clean file types (e.g. "audio,image,txt")
                var fileTypeTokens = fileType
                    .Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
                    .Select(t => t.ToLower())
                    .ToList();

                // Collect all extensions for the provided types
                var allowedExtensions = new HashSet<string>(
                    fileTypeTokens
                        .SelectMany(t => categoryMap.TryGetValue(t, out var exts) ? exts : [t])
                );

                query = query.Where(f => allowedExtensions.Contains(f.FileType.ToLower()));
            }
                
            if (minCreated.HasValue)
            {
                var minCreatedUtc = DateTime.SpecifyKind(minCreated.Value.Date, DateTimeKind.Utc);
                query = query.Where(d => d.TimeCreated >= minCreatedUtc);
            }

            var filesList = await query.ToListAsync();
            return Ok(filesList);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting files.");
            return StatusCode(500, new { error = "An error occurred while retrieving files." });
        }
    }

    // Get a file by ID
    [HttpGet("{id}")]
    [ProducesResponseType(typeof(Files), StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult<Files>> GetFile(int id)
    {
        try
        {
            var file = await _context.Files.FindAsync(id);
            
            if (file == null)
                return NotFound(new { error = $"File with ID {id} not found." });
            
            return Ok(file);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting file with ID {FileID}.", id);
            return StatusCode(500, new { error = "An error occurred while retrieving the file." });
        }
    }

    // Add file to database
    [HttpPost]
    [ProducesResponseType(typeof(object), StatusCodes.Status201Created)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    [ProducesResponseType(StatusCodes.Status409Conflict)]
    public async Task<ActionResult> AddFile([FromBody] FileDto fileDto)
    {
        try
        {
            var validationResults = new List<ValidationResult>();
            var validationContext = new ValidationContext(fileDto);
            bool isValid = Validator.TryValidateObject(fileDto, validationContext, validationResults, true);
            
            if (!isValid)
                return BadRequest(new { errors = validationResults.Select(v => v.ErrorMessage) });
            
            if (!System.IO.File.Exists(fileDto.FilePath))
                return BadRequest(new { error = "File does not exist at the specified path." });

            var file = new Files
            {
                Name = Path.GetFileName(fileDto.FilePath),
                FileType = Path.GetExtension(fileDto.FilePath),
                FilePath = fileDto.FilePath.Replace('\\', '/')
            };

            _context.Files.Add(file);
            await _context.SaveChangesAsync();
            
            return CreatedAtAction(nameof(GetFile), new { id = file.Id }, new
            {
                file.Id,
                file.Name,
                file.FilePath,
                file.TimeCreated
            });
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error adding file to database.");
            return StatusCode(500, new { error = "An error occurred while adding the file." });
        }
    }

    // Delete a file by ID
    [HttpDelete("{id}")]
    [ProducesResponseType(StatusCodes.Status204NoContent)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult> DeleteFile(int id)
    {
        try
        {
            var file = await _context.Files.FindAsync(id);
            
            if (file == null)
                return NotFound(new { error = $"File with ID {id} not found" });
            
            if (System.IO.File.Exists(file.FilePath))
                System.IO.File.Delete(file.FilePath);
            
            _context.Files.Remove(file);
            await _context.SaveChangesAsync();
            return NoContent();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error deleting file with ID {FileID}.", id);
            return StatusCode(500, new { error = "An error occurred while deleting the file." });
        }
    }
    
    [HttpPost("download")]
    public async Task<IActionResult> DownloadFiles([FromBody] DownloadRequestDto request)
    {
        if (request.Ids is null || request.Ids.Count == 0)
            return BadRequest(new { error = "No file IDs provided" });

        var files = await _context.Files
            .Where(f => request.Ids.Contains(f.Id))
            .ToListAsync();

        if (files.Count == 0)
            return NotFound(new { error = "No files found." });

        var (validFiles, missingFiles) = PartitionValidFiles(files);

        if (missingFiles.Count > 0)
            return NotFound(new { error = "Some files were not found.", missingFiles });

        if (validFiles.Count == 1)
            return GetSingleFileResult(validFiles[0]);

        return await GetZipFileResult(validFiles);
    }

    [HttpGet("{id}/download")]
    public async Task<IActionResult> DownloadFile(int id)
    {
        var file = await _context.Files.FindAsync(id);

        if (file == null)
            return NotFound(new { error = $"File with ID {id} not found." });

        if (!System.IO.File.Exists(file.FilePath))
            return NotFound(new { error = "File not found on disk.", file });

        return GetSingleFileResult(file);
    }

    private static (List<Files> validFiles, List<FileInfoDto> missingFiles)
        PartitionValidFiles(IEnumerable<Files> files)
    {
        var valid = new List<Files>();
        var missing = new List<FileInfoDto>();

        foreach (var file in files)
        {
            if (System.IO.File.Exists(file.FilePath))
                valid.Add(file);
            else
                missing.Add(new FileInfoDto
                {
                    Id = file.Id,
                    Name = file.Name,
                    FilePath = file.FilePath
                });
        }

        return (valid, missing);
    }

    private FileResult GetSingleFileResult(Files file)
    {
        var fileName = Path.GetFileName(file.FilePath);
        var stream = new FileStream(file.FilePath, FileMode.Open, FileAccess.Read, FileShare.Read);

        if (!_contentTypeProvider.TryGetContentType(file.FilePath, out var contentType))
            contentType = "application/octet-stream";

        return File(stream, contentType, fileName);  // MVC disposes it after streaming
    }

    private async Task<IActionResult> GetZipFileResult(IEnumerable<Files> files)
    {
        try
        {
            var memoryStream = new MemoryStream();
            await using (var zipArchive = new ZipArchive(memoryStream, ZipArchiveMode.Create, true))
            {
                var usedNames = new HashSet<string>();

                foreach (var file in files)
                {
                    var entryName = $"{file.Name}{file.FileType}";
                    if (!usedNames.Add(entryName))
                        entryName = $"{file.Id}_{entryName}";

                    var entry = zipArchive.CreateEntry(entryName, CompressionLevel.Optimal);

                    await using var entryStream = await entry.OpenAsync();
                    await using var fileStream = new FileStream(file.FilePath, FileMode.Open, FileAccess.Read, FileShare.Read);
                    await fileStream.CopyToAsync(entryStream);
                }
            }
            
            memoryStream.Position = 0;
            return File(memoryStream, "application/zip", $"files_{DateTime.UtcNow:yyyyMMdd_HHmmss}.zip");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error creating ZIP archive.");
            return StatusCode(500, new { error = "An error occurred while creating the ZIP archive." });
        }
    }
}
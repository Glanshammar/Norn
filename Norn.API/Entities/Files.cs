using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace Norn.API.Entities;

public class Files
{
    [Key]
    [Column("id")]
    public int Id { get; set; }
    
    [Required]
    [MinLength(1)]
    [MaxLength(500)]
    [Column("name")]
    public string Name { get; set; } = string.Empty;
    
    [Column("time_created")]
    public DateTime TimeCreated { get; set; } = DateTime.SpecifyKind(DateTime.UtcNow, DateTimeKind.Utc);

    [Column("time_modified")]
    public DateTime TimeModified { get; set; } = DateTime.SpecifyKind(DateTime.UtcNow, DateTimeKind.Utc);
    
    [Required]
    [MinLength(2)]
    [MaxLength(500)]
    [Column("file_path")]
    public string FilePath { get; set; } = string.Empty;
    
    [MaxLength(20)]
    [Column("file_type")]
    public string FileType { get; set; } = string.Empty;
}

public class GoalFile
{
    [Key]
    public int Id { get; set; }
    
    [Required]
    [ForeignKey(nameof(Goal))]
    public int GoalId { get; set; }
    
    [Required]
    [ForeignKey(nameof(File))]
    public int FileId { get; set; }
    
    public virtual Goal Goal { get; set; } = null!;
    public virtual Task File { get; set; } = null!;
}
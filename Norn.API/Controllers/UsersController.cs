using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using Norn.API.Context;
using Norn.API.DTO;
using Norn.API.Entities;

namespace Norn.API.Controllers;

[ApiController]
[Route("api/[controller]")]
public class UsersController : ControllerBase
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<UsersController> _logger;

    public UsersController(ApplicationDbContext context, ILogger<UsersController> logger)
    {
        _context = context;
        _logger = logger;
    }

    // Get all users
    [HttpGet]
    [ProducesResponseType(typeof(IEnumerable<User>), StatusCodes.Status200OK)]
    public async Task<ActionResult<IEnumerable<User>>> GetUsers()
    {
        try
        {
            var users = await _context.Users.ToListAsync();
            return Ok(users);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting users");
            return StatusCode(500, new { error = "An error occurred while retrieving users" });
        }
    }

    // Get user by ID
    [HttpGet("{id}")]
    [ProducesResponseType(typeof(User), StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult<User>> GetUser(int id)
    {
        try
        {
            var user = await _context.Users.FindAsync(id);
            
            if (user == null)
            {
                return NotFound(new { error = $"User with ID {id} not found" });
            }
            
            return Ok(user);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting user with ID {UserId}", id);
            return StatusCode(500, new { error = "An error occurred while retrieving the user" });
        }
    }

    // Create user
    [HttpPost]
    [ProducesResponseType(typeof(object), StatusCodes.Status201Created)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    [ProducesResponseType(StatusCodes.Status409Conflict)]
    public async Task<ActionResult> CreateUser([FromBody] UserDto userDto)
    {
        try
        {
            var existingUser = await _context.Users.FirstOrDefaultAsync(u => u.Email == userDto.Email);
            if (existingUser != null)
            {
                return Conflict(new { error = "A user with this email already exists." });
            }
            
            var validationResults = new List<ValidationResult>();
            var validationContext = new ValidationContext(userDto);
            bool isValid = Validator.TryValidateObject(userDto, validationContext, validationResults, true);
            
            if (!isValid)
            {
                return BadRequest(new { errors = validationResults.Select(v => v.ErrorMessage) });
            }
            
            var user = new User
            {
                Name = userDto.Name,
                Email = userDto.Email,
                CreatedAt = DateTime.UtcNow
            };
            
            user.SetPassword(userDto.Password);
            _context.Users.Add(user);
            await _context.SaveChangesAsync();
            
            return CreatedAtAction(nameof(GetUser), new { id = user.Id }, new
            {
                user.Id,
                user.Name,
                user.Email,
                user.CreatedAt
            });
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error creating user");
            return StatusCode(500, new { error = "An error occurred while creating the user" });
        }
    }

    // Update a user
    [HttpPut("{id}")]
    [ProducesResponseType(StatusCodes.Status204NoContent)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult> UpdateUser(int id, [FromBody] UserDto userDto)
    {
        try
        {
            var user = await _context.Users.FindAsync(id);
            if (user == null)
            {
                return NotFound(new { error = $"User with ID {id} not found" });
            }
            
            if (user.Email != userDto.Email)
            {
                var emailExists = await _context.Users.AnyAsync(u => u.Email == userDto.Email);
                if (emailExists)
                {
                    return Conflict(new { error = "A user with this email already exists." });
                }
            }
            
            var validationResults = new List<ValidationResult>();
            var validationContext = new ValidationContext(userDto);
            bool isValid = Validator.TryValidateObject(userDto, validationContext, validationResults, true);
            
            if (!isValid)
            {
                return BadRequest(new { errors = validationResults.Select(v => v.ErrorMessage) });
            }
            
            user.Name = userDto.Name;
            user.Email = userDto.Email;
            
            if (!string.IsNullOrWhiteSpace(userDto.Password))
            {
                user.SetPassword(userDto.Password);
            }
            
            await _context.SaveChangesAsync();
            
            return NoContent();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error updating user with ID {UserId}", id);
            return StatusCode(500, new { error = "An error occurred while updating the user" });
        }
    }

    // Delete a user
    [HttpDelete("{id}")]
    [ProducesResponseType(StatusCodes.Status204NoContent)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult> DeleteUser(int id)
    {
        try
        {
            var user = await _context.Users.FindAsync(id);
            if (user == null)
            {
                return NotFound(new { error = $"User with ID {id} not found" });
            }
            
            _context.Users.Remove(user);
            await _context.SaveChangesAsync();
            
            return NoContent();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error deleting user with ID {UserId}", id);
            return StatusCode(500, new { error = "An error occurred while deleting the user" });
        }
    }
}
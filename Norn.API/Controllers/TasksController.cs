using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using Norn.API.Context;
using Norn.API.DTO;
using Entities_Task = Norn.API.Entities.Task;
using Task = Norn.API.Entities.Task;

namespace Norn.API.Controllers;

[ApiController]
[Route("api/[controller]")]
public class TasksController : ControllerBase
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<TasksController> _logger;

    public TasksController(ApplicationDbContext context, ILogger<TasksController> logger)
    {
        _context = context;
        _logger = logger;
    }

    //Get tasks
    [HttpGet]
    [ProducesResponseType(typeof(IEnumerable<Task>), StatusCodes.Status200OK)]
    public async Task<ActionResult<IEnumerable<Task>>> GetTasks()
    {
        try
        {
            var tasks = await _context.Tasks
                .Include(t => t.GoalTasks)
                .ToListAsync();
            return Ok(tasks);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting tasks");
            return StatusCode(500, new { error = "An error occurred while retrieving tasks" });
        }
    }

    // Get task by ID
    // api/tasks/{id}
    [HttpGet("{id}")]
    [ProducesResponseType(typeof(Task), StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult<Task>> GetTask(int id)
    {
        try
        {
            var task = await _context.Tasks
                .Include(t => t.GoalTasks)
                .FirstOrDefaultAsync(t => t.Id == id);
            
            if (task == null)
            {
                return NotFound(new { error = $"Task with ID {id} not found" });
            }
            
            return Ok(task);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting task with ID {TaskId}", id);
            return StatusCode(500, new { error = "An error occurred while retrieving the task" });
        }
    }

    // Create task
    [HttpPost]
    [ProducesResponseType(typeof(Task), StatusCodes.Status201Created)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    public async Task<ActionResult<Task>> CreateTask([FromBody] TaskDto taskDto)
    {
        try
        {
            var validationResults = new List<ValidationResult>();
            var validationContext = new ValidationContext(taskDto);
            bool isValid = Validator.TryValidateObject(taskDto, validationContext, validationResults, true);
            
            if (!isValid)
            {
                return BadRequest(new { errors = validationResults.Select(v => v.ErrorMessage) });
            }
            
            var task = new Task
            {
                TaskName = taskDto.TaskName,
                TaskDescription = taskDto.TaskDescription
            };
            
            _context.Tasks.Add(task);
            await _context.SaveChangesAsync();
            
            return CreatedAtAction(nameof(GetTask), new { id = task.Id }, task);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error creating task");
            return StatusCode(500, new { error = "An error occurred while creating the task" });
        }
    }

    // Update task by ID
    [HttpPut("{id}")]
    [ProducesResponseType(StatusCodes.Status204NoContent)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult> UpdateTask(int id, [FromBody] TaskDto taskDto)
    {
        try
        {
            var task = await _context.Tasks.FindAsync(id);
            if (task == null)
            {
                return NotFound(new { error = $"Task with ID {id} not found" });
            }
            
            var validationResults = new List<ValidationResult>();
            var validationContext = new ValidationContext(taskDto);
            bool isValid = Validator.TryValidateObject(taskDto, validationContext, validationResults, true);
            
            if (!isValid)
            {
                return BadRequest(new { errors = validationResults.Select(v => v.ErrorMessage) });
            }
            
            task.TaskName = taskDto.TaskName;
            task.TaskDescription = taskDto.TaskDescription;
            
            await _context.SaveChangesAsync();
            
            return NoContent();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error updating task with ID {TaskId}", id);
            return StatusCode(500, new { error = "An error occurred while updating the task" });
        }
    }

    // Delete task by ID
    [HttpDelete("{id}")]
    [ProducesResponseType(StatusCodes.Status204NoContent)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult> DeleteTask(int id)
    {
        try
        {
            var task = await _context.Tasks.FindAsync(id);
            if (task == null)
            {
                return NotFound(new { error = $"Task with ID {id} not found" });
            }
            
            _context.Tasks.Remove(task);
            await _context.SaveChangesAsync();
            
            return NoContent();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error deleting task with ID {TaskId}", id);
            return StatusCode(500, new { error = "An error occurred while deleting the task" });
        }
    }
}
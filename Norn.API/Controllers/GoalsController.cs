using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using Norn.API.Context;
using Norn.API.DTO;
using Norn.API.Entities;

namespace Norn.API.Controllers;

[ApiController]
[Route("api/[controller]")]
public class GoalsController : ControllerBase
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<GoalsController> _logger;

    public GoalsController(ApplicationDbContext context, ILogger<GoalsController> logger)
    {
        _context = context;
        _logger = logger;
    }
    
    [HttpGet]
    [ProducesResponseType(typeof(IEnumerable<Goal>), StatusCodes.Status200OK)]
    public async Task<ActionResult<IEnumerable<Goal>>> GetGoals()
    {
        try
        {
            var goals = await _context.Goals
                .Include(g => g.GoalTasks)
                .ToListAsync();
            return Ok(goals);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting goals");
            return StatusCode(500, new { error = "An error occurred while retrieving goals" });
        }
    }
    
    [HttpGet("{id}")]
    [ProducesResponseType(typeof(Goal), StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult<Goal>> GetGoal(int id)
    {
        try
        {
            var goal = await _context.Goals
                .Include(g => g.GoalTasks)
                .FirstOrDefaultAsync(g => g.Id == id);
            
            if (goal == null)
                return NotFound(new { error = $"Goal not found" });
            
            return Ok(goal);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting goal with ID {GoalId}", id);
            return StatusCode(500, new { error = "An error occurred while retrieving the goal" });
        }
    }
    
    [HttpPost]
    [ProducesResponseType(typeof(Goal), StatusCodes.Status201Created)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    public async Task<ActionResult<Goal>> CreateGoal([FromBody] GoalDto goalDto)
    {
        try
        {
            var validationResults = new List<ValidationResult>();
            var validationContext = new ValidationContext(goalDto);
            bool isValid = Validator.TryValidateObject(goalDto, validationContext, validationResults, true);
            
            if (!isValid)
                return BadRequest(new { errors = validationResults.Select(v => v.ErrorMessage) });
            
            var goal = new Goal
            {
                Name = goalDto.Name,
                Description = goalDto.Description,
                Priority = goalDto.Priority,
            };
            
            _context.Goals.Add(goal);
            await _context.SaveChangesAsync();
            
            return CreatedAtAction(nameof(GetGoal), new { id = goal.Id }, goal);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error creating goal");
            return StatusCode(500, new { error = "An error occurred while creating the goal" });
        }
    }
    
    [HttpPut("{id}")]
    [ProducesResponseType(StatusCodes.Status204NoContent)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult> UpdateGoal(int id, [FromBody] GoalDto goalDto)
    {
        try
        {
            var goal = await _context.Goals.FindAsync(id);
            if (goal == null)
                return NotFound(new { error = $"Goal with ID {id} not found" });
            
            var validationResults = new List<ValidationResult>();
            var validationContext = new ValidationContext(goalDto);
            bool isValid = Validator.TryValidateObject(goalDto, validationContext, validationResults, true);
            
            if (!isValid)
                return BadRequest(new { errors = validationResults.Select(v => v.ErrorMessage) });
            
            goal.Name = goalDto.Name;
            goal.Description = goalDto.Description;
            goal.Priority = goalDto.Priority;
            
            await _context.SaveChangesAsync();
            
            return NoContent();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error updating goal with ID {GoalId}", id);
            return StatusCode(500, new { error = "An error occurred while updating the goal" });
        }
    }
    
    [HttpDelete("{id}")]
    [ProducesResponseType(StatusCodes.Status204NoContent)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult> DeleteGoal(int id)
    {
        try
        {
            var goal = await _context.Goals.FindAsync(id);
            if (goal == null)
                return NotFound(new { error = $"Goal with ID {id} not found" });
            
            _context.Goals.Remove(goal);
            await _context.SaveChangesAsync();
            
            return NoContent();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error deleting goal with ID {GoalId}", id);
            return StatusCode(500, new { error = "An error occurred while deleting the goal" });
        }
    }
    
    [HttpPost("{goalId}/tasks/{taskId}")]
    [ProducesResponseType(typeof(Goal), StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    [ProducesResponseType(StatusCodes.Status409Conflict)]
    public async Task<ActionResult<Goal>> AddTaskToGoal(int goalId, int taskId)
    {
        try
        {
            var goal = await _context.Goals
                .Include(g => g.GoalTasks)
                .FirstOrDefaultAsync(g => g.Id == goalId);
        
            if (goal == null)
                return NotFound(new { error = $"Goal with ID {goalId} not found" });
        
            var task = await _context.Tasks.FindAsync(taskId);
            if (task == null)
                return NotFound(new { error = $"Task with ID {taskId} not found" });
        
            if (goal.GoalTasks.Any(gt => gt.TaskId == taskId))
                return Conflict(new { error = $"Task with ID {taskId} is already associated with this goal" });
        
            var goalTask = new GoalTask
            {
                GoalId = goalId,
                TaskId = taskId,
                CompletedDate = null
            };
        
            goal.GoalTasks.Add(goalTask);
        
            await _context.SaveChangesAsync();
            return Ok(goal);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error adding task {TaskId} to goal {GoalId}", taskId, goalId);
            return StatusCode(500, new { error = "An error occurred while adding task to goal" });
        }
    }
    
    [HttpDelete("{goalId}/tasks/{taskId}")]
    [ProducesResponseType(typeof(Goal), StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<ActionResult<Goal>> RemoveTaskFromGoal(int goalId, int taskId)
    {
        try
        {
            var goal = await _context.Goals
                .Include(g => g.GoalTasks)
                .FirstOrDefaultAsync(g => g.Id == goalId);
            
            if (goal == null)
                return NotFound(new { error = $"Goal with ID {goalId} not found" });
            
            var task = goal.GoalTasks.FirstOrDefault(t => t.Id == taskId);
            if (task == null)
                return NotFound(new { error = $"Task with ID {taskId} not found in this goal" });
            
            goal.GoalTasks.Remove(task);
            await _context.SaveChangesAsync();
            
            return Ok(goal);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error removing task {TaskId} from goal {GoalId}", taskId, goalId);
            return StatusCode(500, new { error = "An error occurred while removing task from goal" });
        }
    }
}
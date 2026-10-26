using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace Norn.API.Entities;

public enum Priority
{
    Low = 0,
    Medium = 1,
    High = 2,
    Critical = 3,
}

public class TaskStep
{
    [Key]
    public int Id { get; set; }
    
    [Required]
    public int TaskId { get; set; }
    
    [ForeignKey(nameof(TaskId))]
    public virtual Task Task { get; set; } = null!;
    
    [Required]
    [MaxLength(50)]
    public string StepTitle { get; set; } = string.Empty;
    
    public bool IsCompleted { get; set; } = false;
}

public class Task
{
    [Key]
    public int Id { get; set; }
    
    [Required]
    [MaxLength(50)]
    [Column("task_name")]
    public string TaskName { get; set; } = string.Empty;
    
    [Required]
    public Priority Priority { get; set; } = Priority.Medium;

    public DateTime? DueDate { get; set; } = null!;
    
    [Required]
    [MaxLength(500)]
    [Column("task_description")]
    public string TaskDescription { get; set; } = string.Empty;
    
    public bool IsCompleted { get; set; } = false;
    
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime? UpdatedAt { get; set; } = null!;
    
    public virtual ICollection<GoalTask> GoalTasks { get; set; } = new List<GoalTask>();
    public virtual ICollection<TaskStep> TaskSteps { get; set; } = new List<TaskStep>();
}

public class Goal
{
    [Key]
    public int Id { get; set; }
    
    [Required]
    [MaxLength(100)]
    public string Name { get; set; } = string.Empty;
    
    [Required]
    public Priority Priority { get; set; } = Priority.Medium;
    
    [Required]
    public DateTime TargetDate { get; set; }
    
    [MaxLength(500)]
    public string Description { get; set; } = string.Empty;
    
    public bool IsCompleted { get; set; } = false;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime? UpdatedAt { get; set; } =  null;
    
    public GoalCustomData CustomData { get; set; } = new();
    
    public virtual ICollection<GoalTask> GoalTasks { get; set; } = new List<GoalTask>();
}

public class GoalCustomData
{
    public Dictionary<string, string> Fields { get; set; } = new();
}

public class GoalTask
{
    [Key]
    public int Id { get; set; }
    
    [Required]
    [ForeignKey(nameof(Goal))]
    public int GoalId { get; set; }
    
    [Required]
    [ForeignKey(nameof(Task))]
    public int TaskId { get; set; }
    
    public virtual Goal Goal { get; set; } = null!;
    public virtual Task Task { get; set; } = null!;
    
    public DateTime? CompletedDate { get; set; }
}
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace Norn.API.Entities;

public enum MuscleGroup
{
    Chest, Back, Legs, Shoulders, Arms, Core, Glutes, Other
}

public enum WorkoutType
{
    Strength, Cardio, Flexibility, HIIT, Recovery
}

public class Exercise
{
    [Key]
    [Column("id")]
    public int Id { get; set; }
    
    [Required]
    [MaxLength(50, ErrorMessage = "Exercise name cannot exceed 50 characters")]
    [Column("name")]
    public string Name { get; set; } = string.Empty;
    
    [MaxLength(250)]
    [Column("description")]
    public string? Description { get; set; }
    
    [Required]
    [Column("muscle_group")]
    public MuscleGroup MuscleGroup { get; set; }
    
    [MaxLength(50)]
    [Column("equipment")]
    public string? Equipment { get; set; }
}

public class Workout
{
    [Key]
    [Column("id")]
    public int Id { get; set; }
    
    [Column("date_performed")]
    public DateTime DatePerformed { get; set; }
    
    [Column("workout_type")]
    public WorkoutType? WorkoutType { get; set; }
    
    [MaxLength(500)]
    [Column("notes")]
    public string? Notes { get; set; }
    
    public virtual ICollection<WorkoutExercise> WorkoutExercises { get; set; } = new List<WorkoutExercise>();
}

public class WorkoutPlan
{
    [Key]
    [Column("id")]
    public int Id { get; set; }
    
    [Required]
    [MaxLength(50, ErrorMessage = "Workout name cannot exceed 50 characters")]
    [Column("name")]
    public string Name { get; set; } = string.Empty;
    
    [Required]
    [Column("workout_type")]
    public WorkoutType WorkoutType { get; set; }
}

public class WorkoutExercise
{
    [Key]
    [Column("id")]
    public int Id { get; set; }

    [Column("workout_id")]
    public int WorkoutId { get; set; }
    
    [Column("exercise_id")]
    public int ExerciseId { get; set; }

    [ForeignKey("WorkoutId")]
    public virtual Workout Workout { get; set; } = null!;
    
    [ForeignKey("ExerciseId")]
    public virtual Exercise Exercise { get; set; } = null!;

    [Column("sets")]
    public int Sets { get; set; }
    
    [Column("reps")]
    public int Reps { get; set; }
    
    [Column("weight")]
    public double? Weight { get; set; }
    
    [Column("duration")]
    public int? Duration { get; set; }
    
    [Column("goal")]
    public string? Goal { get; set; }
}

public class ExerciseGoal
{
    [Key]
    [Column("id")]
    public int Id { get; set; }

    [Required]
    [Column("exercise_id")]
    public int ExerciseId { get; set; }
    
    [ForeignKey("ExerciseId")]
    public virtual Exercise Exercise { get; set; } = null!;

    [Required]
    [Column("target_weight")]
    public decimal TargetWeight { get; set; }

    [Column("target_reps")]
    public int? TargetReps { get; set; }

    [Column("target_sets")]
    public int? TargetSets { get; set; }

    [Required]
    [Column("deadline")]
    public DateTime Deadline { get; set; }

    [Column("start_date")]
    public DateTime? StartDate { get; set; }

    [Column("completed_date")]
    public DateTime? CompletedDate { get; set; }

    [MaxLength(500)]
    [Column("notes")]
    public string? Notes { get; set; }
    
    [Column("user_id")]
    public int UserId { get; set; }
    
    [ForeignKey("UserId")]
    public virtual User User { get; set; } = null!;
}

public class BodyGoalCurrent
{
    [Key]
    [Column("id")]
    public int Id { get; set; }
    
    [Column("current_weight")]
    public float? CurrentWeight { get; set; } = null!;

    [Column("current_fat_percentage")]
    public float? CurrentFatPercentage { get; set; } = null!;

    [Column("current_muscle_mass")]
    public float? CurrentMuscleMass { get; set; } = null!;

    [Column("current_waist_circumference")]
    public float? CurrentWaistCircumference { get; set; } = null!;
    
    [Column("current_resting_heart_rate")]
    public int? CurrentRestingHeartRate { get; set; } = null!;
    
    [Column("current_date")]
    public DateTime CurrentDate { get; set; } = DateTime.Today;
    
    [Column("goal_notes")]
    [MaxLength(500, ErrorMessage = "Goal notes cannot exceed 500 characters")]
    public string GoalNotes { get; set; } = String.Empty;
}

public class BodyGoalTarget
{
    [Key]
    [Column("id")]
    public int Id { get; set; }

    [Column("target_weight")]
    public float? TargetWeight { get; set; } = null!;

    [Column("target_fat_percentage")]
    public float? TargetFatPercentage { get; set; } = null!;

    [Column("target_muscle_mass")]
    public float? TargetMuscleMass { get; set; } = null!;

    [Column("target_waist_circumference")]
    public float? TargetWaistCircumference { get; set; } = null!;

    [Column("target_resting_heart_rate")]
    public int? TargetRestingHeartRate { get; set; } = null!;

    [Column("deadline_date")]
    public DateTime? DeadlineDate { get; set; } = null!;

    [Column("goal_type")]
    [MaxLength(50, ErrorMessage = "Goal type cannot exceed 50 characters")]
    public string GoalType { get; set; } = String.Empty; // e.g., "Loss", "Gain", "Maintenance"
    
    [Column("goal_notes")]
    [MaxLength(500, ErrorMessage = "Goal notes cannot exceed 500 characters")]
    public string GoalNotes { get; set; } = String.Empty;
}
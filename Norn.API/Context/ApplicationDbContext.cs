using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.ValueGeneration;
using Microsoft.EntityFrameworkCore.ChangeTracking;
using Microsoft.EntityFrameworkCore.Metadata;
using Norn.API.Entities;
using Task = Norn.API.Entities.Task;
using Tasks = Norn.API.Entities.Task;

namespace Norn.API.Context;

public partial class ApplicationDbContext : DbContext
{
    public ApplicationDbContext()
    {
    }

    public ApplicationDbContext(DbContextOptions<ApplicationDbContext> options)
        : base(options)
    {
    }

    public DbSet<User> Users => Set<User>();
    public DbSet<Files> Files => Set<Files>();
    public DbSet<Goal> Goals => Set<Goal>();
    public DbSet<Task> Tasks => Set<Task>();
    public DbSet<GoalTask> GoalTasks => Set<GoalTask>();
    public DbSet<TaskStep> TaskSteps => Set<TaskStep>();

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        modelBuilder.Entity<GoalTask>()
            .HasOne(gt => gt.Goal)               // GoalTask has one Goal
            .WithMany(g => g.GoalTasks)     // Goal has many GoalTasks
            .HasForeignKey(gt => gt.GoalId)      // Foreign key is GoalId
            .OnDelete(DeleteBehavior.Cascade);   // Optional: If Goal is deleted, delete its links

        modelBuilder.Entity<GoalTask>()
            .HasOne(gt => gt.Task)               // GoalTask has one Task
            .WithMany(t => t.GoalTasks)     // Task has many GoalTasks
            .HasForeignKey(gt => gt.TaskId)      // Foreign key is TaskId
            .OnDelete(DeleteBehavior.Cascade);   // Optional: If Task is deleted, delete its links
        
        modelBuilder.Entity<TaskStep>()
            .HasOne(ts => ts.Task)           // TaskStep has one Task
            .WithMany(t => t.TaskSteps)      // Task has many TaskSteps
            .HasForeignKey(ts => ts.TaskId)  // Foreign key is TaskId
            .OnDelete(DeleteBehavior.Cascade); // Delete steps when Task is deleted
        
        modelBuilder.Entity<Goal>()
            .OwnsOne(g => g.CustomData, owned =>
            {
                owned.ToJson();
            });
        
        // Indexes
        modelBuilder.Entity<GoalTask>()
            .HasIndex(gt => new { gt.GoalId, gt.TaskId })
            .IsUnique();

        modelBuilder.Entity<Task>()
            .HasIndex(t => t.DueDate);

        modelBuilder.Entity<TaskStep>()
            .HasIndex(ts => ts.TaskId);
        
        modelBuilder.Entity<GoalTask>().ToTable("GoalTasks");
        
        foreach (var entityType in modelBuilder.Model.GetEntityTypes())
        {
            foreach (var property in entityType.GetProperties())
            {
                if (property.ClrType == typeof(Guid) && property.ValueGenerated == ValueGenerated.OnAdd)
                {
                    property.SetValueGeneratorFactory((_, _) => new Version7GuidValueGenerator());
                }
            }
        }
    }
}

public class Version7GuidValueGenerator : ValueGenerator<Guid>
{
    public override Guid Next(EntityEntry entry) => Guid.CreateVersion7();
    public override bool GeneratesTemporaryValues => false;
}
using System.Net;
using Microsoft.EntityFrameworkCore;
using Norn.API.Context;

var builder = WebApplication.CreateBuilder(args);

builder.WebHost.ConfigureKestrel(options =>
{
    var httpUrl = Environment.GetEnvironmentVariable("ASPNETCORE_URL") ?? 
                  builder.Configuration.GetValue<string>("Kestrel:Endpoints:Http:Url") ?? 
                  "http://0.0.0.0:5000";

    if (Uri.TryCreate(httpUrl, UriKind.Absolute, out var uri))
    {
        var ip = IPAddress.Parse(uri.Host == "*" || string.IsNullOrEmpty(uri.Host) ? "0.0.0.0" : uri.Host);
        options.Listen(ip, uri.Port);
    }
});

builder.Services.AddOpenApi();
builder.Services.AddDbContext<ApplicationDbContext>(options =>
    options.UseNpgsql(builder.Configuration.GetConnectionString("DevelopmentConnection")));

builder.Services.AddControllers();
builder.Services.AddCors(options =>
{
    options.AddPolicy("AllowAll", policy =>
        policy.AllowAnyOrigin()
            .AllowAnyMethod()
            .AllowAnyHeader()
            .WithExposedHeaders("Content-Disposition"));
});

var app = builder.Build();

// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
    
    using var scope = app.Services.CreateScope();
    var dbContext = scope.ServiceProvider.GetRequiredService<ApplicationDbContext>();
    await dbContext.Database.MigrateAsync();
}

app.UseCors("AllowAll");
app.MapControllers();
app.UseHttpsRedirection();

app.MapGet("/api/status", () => Results.Ok(new { 
        status = "ok", 
        message = "API is online",
        timestamp = DateTime.UtcNow
    }))
    .WithName("GetApiStatus");

await app.RunAsync();
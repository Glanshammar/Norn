using System;
using System.ComponentModel;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using Konscious.Security.Cryptography;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json.Serialization;

namespace Norn.API.Entities;

public class User
{
    [Key]
    [Column("id")]
    public Guid Id { get; set; } 

    [Required]
    [MaxLength(100)]
    [MinLength(1)]
    [Column("name")]
    public string Name { get; set; } = string.Empty;
    
    [Required]
    [MaxLength(100)]
    [MinLength(1)]
    [EmailAddress]
    [Column("email")]
    public string Email { get; set; } = string.Empty;
    
    [Required]
    [MaxLength(128)] // Increased for salt(16)+hash(32)+base64 overhead
    [JsonIgnore]
    [Column("password")]
    public string PasswordHash { get; private set; } = string.Empty;
    
    [Column("created_at")]
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    
    [NotMapped]
    [JsonIgnore]
    public string Password 
    { 
        get => throw new InvalidOperationException("Use SetPassword(string) instead.");
        private set => throw new InvalidOperationException("Use SetPassword(string) instead.");
    }
    
    public void SetPassword(string plaintextPassword)
    {
        byte[] salt = RandomNumberGenerator.GetBytes(16);
        using var argon2 = new Argon2id(Encoding.UTF8.GetBytes(plaintextPassword))
        {
            Salt = salt,
            DegreeOfParallelism = 4,  // Threads
            Iterations = 4,
            MemorySize = 65536        // 64MB
        };
        byte[] hash = argon2.GetBytes(32);
    
        byte[] combined = new byte[48];
        salt.CopyTo(combined, 0);
        hash.CopyTo(combined, 16);
        PasswordHash = Convert.ToBase64String(combined);
    }

    public bool VerifyPassword(string plaintextPassword)
    {
        byte[] combined = Convert.FromBase64String(PasswordHash);
        byte[] salt = new byte[16];
        Array.Copy(combined, 0, salt, 0, 16);
    
        using var argon2 = new Argon2id(Encoding.UTF8.GetBytes(plaintextPassword))
        {
            Salt = salt,
            DegreeOfParallelism = 4,
            Iterations = 4,
            MemorySize = 65536
        };
        byte[] hash = argon2.GetBytes(32);
        return CryptographicOperations.FixedTimeEquals(hash, combined.AsSpan(16));
    }
}
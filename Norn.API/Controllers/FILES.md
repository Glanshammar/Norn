# Files API Endpoint Documentation

This document describes the `FilesController` endpoints in the Yggdrasil.API service. These endpoints manage file metadata in the database and support downloading single files or multiple files as a ZIP archive.

All routes are under the base path:  
`/api/files` (i.e., the controller is routed as `api/[controller]` → `files`).

---

## 1. Add a file to the database

Adds a file’s metadata entry to the database. The file must already exist on disk at the given path.

- **HTTP Method:** `POST`
- **URL:** `/api/files`
- **Request body (JSON):**
  ```json
  {
    "filePath": "path/myfile.pdf"
  }
  ```
- **Validation:**
    - `filePath` must be non‑null and valid according to `DataAnnotations`.
- **Responses:**
    - `201 Created` – File metadata added; returns:
      ```json
      {
        "id": 42,
        "name": "myfile.pdf",
        "filePath": "path/myfile.pdf",
        "timeCreated": "2026-04-27T18:30:00Z"
      }
      ```
    - `400 Bad Request` – Invalid `filePath` or not in allowed directory.
    - `409 Conflict` – If your DTO validation includes a `Conflict` case (otherwise `400` or `500`).
    - `500 Internal Server Error` – Unexpected server error.

---

## 2. List all (or filtered) files

Retrieves a list of file metadata, optionally filtered by search text, file type, or minimum creation date.

- **HTTP Method:** `GET`
- **URL:** `/api/files`
- **Query parameters (all optional):**
    - `search` – Text to search in `Name` or `FilePath` (SQL `LIKE` pattern):
        - Example: `/api/files?search=myfile`
    - `fileType` – Filter by category or extension:
        - Categories: `audio`, `image`, `video`, `document`
        - Example: `/api/files?fileType=audio,image`  
          This returns files with extensions like `.mp3`, `.wav`, `.jpg`, `.png`.
        - You can also mix extensions: `audio,.txt` → audio plus `.txt`.
    - `minCreated` – Only files created on or after this UTC‑day:
        - Example: `/api/files?minCreated=2026-04-27`
- **Response:**
    - `200 OK` – Returns an array of `Files` objects:
      ```json
      [
        {
          "id": 42,
          "name": "myfile.pdf",
          "fileType": ".pdf",
          "filePath": "path/myfile.pdf",
          "timeCreated": "2026-04-27T18:30:00Z"
        }
      ]
      ```

---

## 3. Get a specific file by ID

Returns the metadata of a single file by its database ID.

- **HTTP Method:** `GET`
- **URL:** `/api/files/{id}`  
  Example: `/api/files/42`
- **Response:**
    - `200 OK` – Returns the `Files` object.
    - `404 Not Found` – File with that ID does not exist.
    - `500 Internal Server Error` – Unexpected server error.

---

## 4. Delete a file (metadata + disk)

Removes the file’s metadata from the database and deletes the physical file from disk.

- **HTTP Method:** `DELETE`
- **URL:** `/api/files/{id}`  
  Example: `/api/files/42`
- **Behavior:**
    - If the file exists in the DB, it deletes the row and the file on disk.
    - If the disk file is missing, it still deletes the row.
- **Response:**
    - `204 No Content` – Deleted successfully.
    - `404 Not Found` – File with that ID does not exist.
    - `500 Internal Server Error` – Unexpected server error.

---

## 5. Download a single file by ID

Triggers a browser‑style download of a file by its database ID.

- **HTTP Method:** `GET`
- **URL:** `/api/files/{id}/download`  
  Example: `/api/files/42/download`
- **Behavior:**
    - Checks the DB; if ID not found → `404`.
    - Checks disk; if file not found → `404` with a message about the file missing on disk.
    - Otherwise streams the file with correct `Content-Type` (or `application/octet-stream` if unknown).
- **Response:**
    - `200 OK` – File stream with `Content-Disposition` attachment.
    - `404 Not Found` – File metadata or disk file missing.
    - `500 Internal Server Error` – Unexpected server error.

---

## 6. Download multiple files as a ZIP

Downloads one or more files as a ZIP archive. Can be used to download a single file or multiple files.

- **HTTP Method:** `POST`
- **URL:** `/api/files/download`
- **Request body (JSON):**
  ```json
  {
    "ids": [1, 2, 3]
  }
  ```
- **Rules:**
    - `ids` must not be null or empty.
    - The server checks which files exist on disk and which do not.
- **Response behavior:**
    - `400 Bad Request` – No IDs provided (`"No file IDs provided"`).
    - `404 Not Found` – None of the IDs exist.
    - `404 Not Found` (with `missingFiles`) – Some IDs exist, some don’t:
      ```json
      {
        "error": "Some files were not found.",
        "missingFiles": [
          { "id": 44, "name": "missing.pdf", "filePath": "path/missing.pdf" }
        ]
      }
      ```
    - If only **one valid file** → same behavior as `/{id}/download` (single file stream).
    - If **two or more valid files** → returns a ZIP with content type `application/zip` and filename like:
      `files_20260427_183000.zip`.

---

## Example usage (curl)

- List all files:
  ```bash
  curl http://localhost:5000/api/files
  ```
- Filter by type:
  ```bash
  curl 'http://localhost:5000/api/files?fileType=image'
  ```
- Download by ID:
  ```bash
  curl -OJ http://localhost:5000/api/files/42/download
  ```
- Download multiple as ZIP:
  ```bash
  curl -X POST http://localhost:5000/api/files/download \
    -H "Content-Type: application/json" \
    -d '{"ids": [1, 2, 3] }'
  ```
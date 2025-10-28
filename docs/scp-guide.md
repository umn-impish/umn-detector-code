# How to Copy Files with SCP (Secure Copy)

SCP (Secure Copy) is a command-line tool that lets you securely transfer files between your local computer and a remote server over SSH.

This guide will show you how to use SCP step-by-step — even if you’ve never used it before.

---

## 🧠 What You Need to Know First

SCP follows this basic structure:

```
scp [options] SOURCE DESTINATION
```

- **SOURCE** is where the file(s) currently are.
- **DESTINATION** is where you want to copy them.
- Either the source or the destination can be **remote**.
- Remote locations are written as:
  ```
  username@hostname:/path/to/file
  ```
  Example:
  ```
  alice@example.com:/home/alice/file.txt
  ```

If you’re copying **to** a remote system, SCP will prompt for your SSH password unless you’ve set up key-based authentication.

---

## 🗂️ 1. Copy a Single File

### Copy from local → remote
```bash
scp myfile.txt alice@example.com:/home/alice/
```

This copies `myfile.txt` from your computer to `/home/alice/` on the remote host.

### Copy from remote → local
```bash
scp alice@example.com:/home/alice/myfile.txt .
```

The `.` means “copy it to the current directory on my computer.”

---

## 📁 2. Copy an Entire Directory

By default, SCP only copies individual files.  
To copy a whole folder (recursively), use the **`-r`** option.

### Copy directory from local → remote
```bash
scp -r myfolder alice@example.com:/home/alice/
```

### Copy directory from remote → local
```bash
scp -r alice@example.com:/home/alice/myfolder .
```

This will copy all files and subfolders.

---

## ✳️ 3. Copy Multiple Files with Wildcards

Wildcards (`*`, `?`) can match groups of files.

### Copy multiple local files to remote
```bash
scp *.txt alice@example.com:/home/alice/textfiles/
```
This copies **all `.txt` files** in the current folder.

### Copy specific pattern from remote → local
```bash
scp alice@example.com:/home/alice/*.log ./logs/
```
This copies all `.log` files from the remote directory into your local `logs` folder.

> 💡 Tip: When using wildcards for remote paths, wrap them in quotes:
> ```bash
> scp "alice@example.com:/home/alice/*.log" ./logs/
> ```
>
> Otherwise, your local shell may try to expand the `*` before sending it.

---

## ⚙️ 4. Useful Options

| Option | Description |
|--------|--------------|
| `-r` | Copy directories recursively |
| `-p` | Preserve modification times and permissions |
| `-v` | Verbose mode (shows progress and debug info) |
| `-C` | Compress data during transfer (useful for slow connections) |

Example:
```bash
scp -rvC myfolder alice@example.com:/home/alice/
```

---

## ✅ Summary

| Task | Command |
|------|----------|
| Copy single file (local → remote) | `scp file.txt user@host:/path/` |
| Copy single file (remote → local) | `scp user@host:/path/file.txt .` |
| Copy directory | `scp -r folder user@host:/path/` |
| Copy multiple files | `scp *.txt user@host:/path/` |
| Copy with compression | `scp -C file.txt user@host:/path/` |

---

## 🧩 Troubleshooting

- **Permission denied** → You might not have write permission on the destination folder.
- **Connection timed out** → The server may be down, or the hostname is wrong.
- **Host key verification failed** → Your system doesn’t recognize the remote host; try connecting once with plain `ssh` to confirm.
- **Stuck transfer** → Try adding `-v` to see what’s happening behind the scenes.

---

**That’s it!** You now know how to use SCP to copy files, directories, and groups of files between systems securely.

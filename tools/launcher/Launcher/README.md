# CDDA Fork Launcher

A small Windows launcher for Xbro10's fork of Cataclysm: Dark Days Ahead. It installs and updates the game,
keeps your saves safe, and lets you play with one click.

## Setting it up

1. Make a folder with a short path, for example `C:\Games\CDDA` (not inside Program Files).
2. Copy `Start CDDA Launcher.bat` and the `Launcher` folder (from `tools/launcher/` in the fork) into it.
3. Double-click **`Start CDDA Launcher.bat`**. On the Folders tab you can also make a desktop shortcut.

## What it does

- **Game builds tab:**
  - **Builds from** picks where builds come from:
    - **Xbro10 tester builds** (default): the fork's `testing-build-*` releases, which are CDDA experimental plus
      Backrooms Expanded and Survivor Minds.
    - **Official CDDA experimental**: CleverRaven's releases, with a choice of *Graphics* or *Graphics + sounds*.
  - When a newer build is out, the launcher tells you on start and offers to install it.
  - **Install / update** downloads the selected build. **What changed since my build?** lists every commit
    between your build and the selected one.
- **Mods tab:**
  - Add a mod from a GitHub link or a zip file. Removed mods go to the Recycle Bin.
  - **Sync my mods from my fork** (only shown when the fork's source folder is on this PC) copies mod folders that
    are in the fork's `data\mods` but not in the installed game.
- **Folders & backups tab:**
  - Open the game, user data and backup folders.
  - Back up your saves to a zip, or swap back to the previous build.
  - Create a desktop shortcut.
  - **Check for a launcher update.** The launcher also checks by itself after loading the build list. Updates come
    from `tools/launcher` on the fork's `testing` branch. The old copy is kept as `cdda-launcher.ps1.bak`.
- **PLAY** starts the game with `--userdir` set to `UserData`.

## Folder layout

| Folder | What's in it | Changed by updates? |
|---|---|---|
| `Game\` | the installed build | replaced |
| `Game_previous\` | the build before (for rollback) | replaced |
| `UserData\` | saves, config, templates, fonts, **mods** | never |
| `Downloads\` | the newest build zip (older ones are deleted) | yes |
| `Backups\` | save backups | never |

The launcher never writes into a fork checkout. It only copies mods out of one. When it runs from inside the
fork's source (`tools\launcher`), it doesn't update itself; pull the fork instead.

## Command line

```bat
powershell -ExecutionPolicy Bypass -File cdda-launcher.ps1 -InstallNewest
powershell -ExecutionPolicy Bypass -File cdda-launcher.ps1 -AddMod https://github.com/owner/repo
powershell -ExecutionPolicy Bypass -File cdda-launcher.ps1 -SelfTest
```

`-InstallNewest` uses the source picked in the window (tester builds unless you changed it).
Settings are kept in `settings.json` next to the script.

## For the fork's maintainer

- Publishing a tester build: a GitHub release on `Xbro10/Cataclysm-DDA` with tag `testing-build-YYYY-MM-DD`
  (add `-2`, `-3` for more builds the same day) and one asset named `CDDA-Xbro10-testing-*.zip`. Delete
  `data/cache` before zipping.
- Publishing a launcher update: change the script on `dev`, raise `$script:LauncherVersion` (a date, compared as
  text), and merge it through to `testing`. Every launcher picks it up on its next start.

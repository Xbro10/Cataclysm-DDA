# CDDA Fork Launcher

A small Windows launcher for Xbro10's fork of Cataclysm: Dark Days Ahead. It installs and updates the game,
keeps your saves safe, and lets you play with one click.

## Setting it up

1. Make a folder with a short path, for example `C:\Games\CDDA` (not inside Program Files).
2. Copy `CDDA Fork Launcher.exe` and the `Launcher` folder (from `tools/launcher/` in the fork) into it.
3. Double-click **`CDDA Fork Launcher.exe`**. If Windows SmartScreen warns you, click **More info > Run anyway**
   (the exe isn't signed). **Settings > Create desktop shortcut** puts it on your desktop.

The .exe only starts `Launcher\cdda-launcher.ps1` without a console window; its source is in `tools/launcher/src/`.
An older folder with `Start CDDA Launcher.bat` still works, and the next launcher update adds the .exe to it.

## What it does

The menu on the left switches between five pages. **PLAY** (bottom right) starts the game with `--userdir` set
to `UserData`.

- **Game:**
  - **Builds from** picks where builds come from:
    - **Xbro10 tester builds** (default): the fork's `testing-build-*` releases, which are CDDA experimental plus
      Backrooms Expanded and Survivor Minds.
    - **Official CDDA experimental**: CleverRaven's releases (these also come with sounds; pick that in Settings).
  - When a newer build is out, the launcher tells you on start and offers to install it.
  - **Install / update** (or double-click a build) downloads it. **What changed since my build?** lists every
    commit between your build and the selected one.
- **Mods:**
  - Add a mod from a GitHub link or a zip file. Removed mods go to the Recycle Bin.
  - **Sync my mods from my fork** (only shown when the fork's source folder is on this PC) copies mod folders that
    are in the fork's `data\mods` but not in the installed game.
- **Sound:** download and update sound packs into `UserData\sound` (game updates never touch them):
  **CC-Sounds** (music and effects, from its newest GitHub release) and **@'s soundpack** (damalsk, 2019).
  **Use this pack** sets the game's sound pack option (`SOUNDPACKS` in `UserDatanfigoptions.json`);
  **Menu sounds only** switches back to the game's own pack. Removed packs go to the Recycle Bin.
- **Saves & backups:** open the save and backup folders, back up your saves to a zip, or swap back to the
  previous build.
- **Settings:**
  - **Theme:** Black (default), Dark blue or Light. It changes straight away.
  - **Download:** Graphics, or Graphics + sounds (official builds only).
  - Close the launcher when the game starts.
  - Look for new game builds, and for launcher updates, when the launcher opens (both on by default).
  - **Check now** for a launcher update. Updates come from `tools/launcher` on the fork's `testing` branch, with the
    icon files. The old script is kept as `cdda-launcher.ps1.bak`.
  - Where your mods come from (developers only), and **Create desktop shortcut** (it uses the launcher's icon).

The icon is `Launcher\launcher.ico` (window, taskbar, shortcut) and `Launcher\launcher-icon.png` (the sidebar).
If either is missing, the launcher downloads it again on its next update check.

## Folder layout

| Folder | What's in it | Changed by updates? |
|---|---|---|
| `Game\` | the installed build | replaced |
| `Game_previous\` | the build before (for rollback) | replaced |
| `UserData\` | saves, config, templates, fonts, **mods**, **sound packs** | never |
| `Downloads\` | the newest build zip (older ones are deleted) | yes |
| `Backups\` | save backups | never |

The launcher never writes into a fork checkout. It only copies mods out of one. When it runs from inside the
fork's source (`tools\launcher`), it doesn't update itself; pull the fork instead.

## Command line

```bat
powershell -ExecutionPolicy Bypass -File cdda-launcher.ps1 -InstallNewest
powershell -ExecutionPolicy Bypass -File cdda-launcher.ps1 -AddMod https://github.com/owner/repo
powershell -ExecutionPolicy Bypass -File cdda-launcher.ps1 -InstallSound cc
powershell -ExecutionPolicy Bypass -File cdda-launcher.ps1 -SelfTest
```

`-InstallNewest` uses the source picked in the window (tester builds unless you changed it).
`-InstallSound` installs or updates a sound pack (`cc` = CC-Sounds, `at` = @'s soundpack) and makes the game use it.
Settings are kept in `settings.json` next to the script.

## For the fork's maintainer

- Publishing a tester build: a GitHub release on `Xbro10/Cataclysm-DDA` with tag `testing-build-YYYY-MM-DD`
  (add `-2`, `-3` for more builds the same day) and one asset named `CDDA-Xbro10-testing-*.zip`. Delete
  `data/cache` before zipping.
- Publishing a launcher update: change the script on `dev`, raise `$script:LauncherVersion` (a date, compared as
  text; add `.2`, `.3` for more the same day), and merge it through to `testing`. Every launcher picks it up on
  its next start. Files listed in `$script:LauncherAssets` (the icon) are downloaded along with it.
- Changing the icon: replace `launcher-icon.png` and `launcher.ico` in `tools/launcher/Launcher/`, run
  `tools/launcher/src/build-exe.ps1` so the .exe gets the new icon, and raise the version.
- Testing the window: `-Screenshot out.png` saves a picture of it and closes; add `-Page game|mods|saves|settings`
  and `-Theme black|blue|light` to pick what it shows.

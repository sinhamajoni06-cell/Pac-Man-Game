#!/usr/bin/env node
'use strict';

// Word-Man command line tool.
// It downloads the official Windows installer from the GitHub release, checks its SHA-256
// hash and runs it. No other packages are needed.

const fs = require('fs');
const os = require('os');
const path = require('path');
const http = require('http');
const https = require('https');
const crypto = require('crypto');
const { spawn, execFileSync } = require('child_process');

const pkg = require('../package.json');
const release = require('../lib/release.json');

const APP_ID = '{9F7084C6-2FC9-417B-9A2B-330665BA6D59}_is1';   // same id as installer/Wordman.iss
const EXE_NAME = 'Wordman.exe';

function fail(message) {
  console.error('wordman: ' + message);
  process.exit(1);
}

function requireWindows() {
  if (process.platform !== 'win32') fail('Word-Man is a Windows game. This command only works on Windows.');
}

// ------------------------------------------------------------------ download

function download(url, dest, redirects = 5) {
  return new Promise((resolve, reject) => {
    const lib = url.startsWith('https:') ? https : http;
    const req = lib.get(url, { headers: { 'User-Agent': 'wordman-cli/' + pkg.version } }, (res) => {
      if ([301, 302, 303, 307, 308].includes(res.statusCode) && res.headers.location) {
        res.resume();
        if (redirects <= 0) return reject(new Error('Too many redirects'));
        return resolve(download(new URL(res.headers.location, url).toString(), dest, redirects - 1));
      }
      if (res.statusCode !== 200) {
        res.resume();
        return reject(new Error('Download failed: HTTP ' + res.statusCode));
      }

      const total = Number(res.headers['content-length'] || 0);
      const hash = crypto.createHash('sha256');
      const out = fs.createWriteStream(dest);
      let done = 0;
      let lastPct = -1;

      res.on('data', (chunk) => {
        hash.update(chunk);
        done += chunk.length;
        if (total && process.stdout.isTTY) {
          const pct = Math.floor((done * 100) / total);
          if (pct !== lastPct) {
            process.stdout.write('\rDownloading... ' + pct + '%');
            lastPct = pct;
          }
        }
      });
      res.on('error', reject);
      out.on('error', reject);
      out.on('finish', () => {
        out.close(() => {
          if (process.stdout.isTTY && total) process.stdout.write('\n');
          resolve(hash.digest('hex'));
        });
      });
      res.pipe(out);
    });
    req.on('error', reject);
  });
}

// Downloads the installer into `folder` and checks its hash. Returns the file path.
async function fetchInstaller(folder) {
  if (!release.url || !release.sha256 || !release.file) {
    fail('this copy of the package has no release information (it was not built by the publish workflow).');
  }
  fs.mkdirSync(folder, { recursive: true });
  const file = path.join(folder, release.file);

  console.log('Downloading ' + release.file + ' ...');
  let sha;
  try {
    sha = await download(release.url, file);
  } catch (err) {
    try { fs.rmSync(file, { force: true }); } catch (_) { /* ignore */ }
    fail(err.message);
  }

  if (sha.toLowerCase() !== String(release.sha256).toLowerCase()) {
    try { fs.rmSync(file, { force: true }); } catch (_) { /* ignore */ }
    fail('the downloaded file does not match the expected SHA-256 hash. It was deleted and NOT run.');
  }
  console.log('Checksum OK (sha256 ' + sha.slice(0, 12) + '...)');
  return file;
}

// ------------------------------------------------------------------ finding the installed game

function readRegistry(hive, valueName) {
  const key = hive + '\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\' + APP_ID;
  try {
    const out = execFileSync('reg', ['query', key, '/v', valueName], {
      encoding: 'utf8',
      stdio: ['ignore', 'pipe', 'ignore']
    });
    const m = out.match(new RegExp(valueName + '\\s+REG_\\w+\\s+(.+)'));
    return m ? m[1].trim() : null;
  } catch (_) {
    return null;
  }
}

function findInstall() {
  for (const hive of ['HKCU', 'HKLM']) {
    const dir = readRegistry(hive, 'InstallLocation');
    if (dir && fs.existsSync(path.join(dir, EXE_NAME))) {
      return { dir, exe: path.join(dir, EXE_NAME), hive };
    }
  }
  const guesses = [
    path.join(process.env.LOCALAPPDATA || '', 'Programs', 'Word-Man'),
    path.join(process.env.ProgramFiles || '', 'Word-Man')
  ];
  for (const dir of guesses) {
    if (fs.existsSync(path.join(dir, EXE_NAME))) return { dir, exe: path.join(dir, EXE_NAME), hive: null };
  }
  return null;
}

// ------------------------------------------------------------------ commands

async function cmdInstall(flags) {
  requireWindows();
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'wordman-'));
  const file = await fetchInstaller(tmp);

  const args = ['/SP-'];
  if (flags.has('--silent')) args.push('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART');
  if (flags.has('--all-users')) args.push('/ALLUSERS');

  console.log('Starting the installer...');
  const child = spawn(file, args, { stdio: 'inherit' });
  child.on('error', (err) => fail('could not start the installer: ' + err.message));
  child.on('exit', (code) => {
    try { fs.rmSync(tmp, { recursive: true, force: true }); } catch (_) { /* ignore */ }
    if (code === 0) console.log('Done. Start the game with:  wordman play');
    process.exit(code === null ? 1 : code);
  });
}

async function cmdDownload(folderArg) {
  const folder = path.resolve(folderArg || '.');
  const file = await fetchInstaller(folder);
  console.log('Saved: ' + file);
}

function cmdPlay() {
  requireWindows();
  const install = findInstall();
  if (!install) fail('Word-Man is not installed. Run:  wordman install');
  console.log('Starting ' + install.exe);
  const child = spawn(install.exe, [], { cwd: install.dir, detached: true, stdio: 'ignore' });
  child.on('error', (err) => fail('could not start the game: ' + err.message));
  child.unref();
}

function cmdUninstall() {
  requireWindows();
  let command = null;
  for (const hive of ['HKCU', 'HKLM']) {
    command = readRegistry(hive, 'UninstallString');
    if (command) break;
  }
  if (!command) fail('Word-Man is not installed.');
  const exe = command.replace(/^"|"$/g, '');
  console.log('Starting the uninstaller...');
  const child = spawn(exe, [], { stdio: 'inherit' });
  child.on('error', (err) => fail('could not start the uninstaller: ' + err.message));
  child.on('exit', (code) => process.exit(code === null ? 1 : code));
}

function cmdInfo() {
  console.log('wordman command line tool ' + pkg.version);
  console.log('Release : ' + (release.tag || '(not set)'));
  console.log('Installer: ' + (release.file || '(not set)'));
  if (process.platform === 'win32') {
    const install = findInstall();
    console.log('Installed: ' + (install ? install.exe : 'no'));
  }
}

function cmdHelp() {
  console.log([
    'Word-Man ' + pkg.version + ' - Pac-Man meets word finding (Windows)',
    '',
    'Usage: wordman <command> [options]',
    '',
    'Commands:',
    '  install [--silent] [--all-users]   download the installer, check it and run it',
    '  download [folder]                  only download and check the installer (any system)',
    '  play                               start the installed game',
    '  uninstall                          remove the game',
    '  info                               show versions and where the game is installed',
    '  help                               show this text',
    '',
    'Options for install:',
    '  --silent      no questions, install straight away',
    '  --all-users   install for every user (asks for administrator rights)'
  ].join('\n'));
}

// ------------------------------------------------------------------ main

async function main() {
  const args = process.argv.slice(2);
  const flags = new Set(args.filter((a) => a.startsWith('--')));
  const rest = args.filter((a) => !a.startsWith('--'));
  const command = rest[0] || 'help';

  switch (command) {
    case 'install':   return cmdInstall(flags);
    case 'download':  return cmdDownload(rest[1]);
    case 'play':      return cmdPlay();
    case 'uninstall': return cmdUninstall();
    case 'info':      return cmdInfo();
    case 'help':
    case '--help':
    case '-h':        return cmdHelp();
    default:
      console.error('wordman: unknown command "' + command + '"\n');
      cmdHelp();
      process.exit(1);
  }
}

main().catch((err) => fail(err && err.message ? err.message : String(err)));

import { app, shell, BrowserWindow, nativeTheme, ipcMain, session, Menu, dialog } from 'electron'
import { join } from 'path'
import { electronApp, optimizer, is } from '@electron-toolkit/utils'
import { release } from 'node:os'
import { store, config } from './store'
import { URL } from 'url'
import { spawn, exec } from 'child_process'

const colors = require('colors')
const formatUnicorn = require('format-unicorn/safe')
const iconv = require('iconv-lite')
const windowStateKeeper = require('electron-window-state')

const VITE_PUBLIC = join(join(__dirname, ".."), "../public")
config.set('appVersion', app.getVersion())

// enable darkmode for electron at startup
nativeTheme.themeSource = store.get('main.isDarkTheme') ? 'dark' : 'light'
const isDevelopment = import.meta.env.MODE === 'development'

// Disable GPU Acceleration for Windows 7
if (release().startsWith('6.1')) app.disableHardwareAcceleration()

// Set application name for Windows 10+ notifications
if (process.platform === 'win32') app.setAppUserModelId(app.getName())

if (!app.requestSingleInstanceLock()) {
  app.quit()
  process.exit(0)
}

let mainWindow = null

function createWindow(): void {

  session.defaultSession.webRequest.onHeadersReceived((details, callback) => {
    if((details.url.startsWith('https://192.168') || details.url.startsWith('https://127.0')) && details.responseHeaders && details.responseHeaders['Set-Cookie']){
      for (let i = 0; i < details.responseHeaders['Set-Cookie'].length; i++)
        details.responseHeaders['Set-Cookie'][i] += "; SameSite=None; Secure";
    }
    callback({ responseHeaders: details.responseHeaders });
  })

  session.defaultSession.webRequest.onBeforeSendHeaders((details, callback) => {
    // set custom User-Agent in requestHeaders
    details.requestHeaders['User-Agent'] = `Kitsu publisher ${app.getVersion()}`
    // set custom Authorization header with access_token
    try {
      const url = new URL(store.get('login.server'))
      if (
        details.url.startsWith(String(url)) &&
        store.get('login.access_token') !== ''
      ) {
        details.requestHeaders['Authorization'] = `Bearer ${store.get(
          'login.access_token'
        )}`
      }
    } catch (error) {
      // do nothing
    }

    callback({ cancel: false, requestHeaders: details.requestHeaders })
  })

  let mainWindowState = windowStateKeeper({
    defaultWidth: 1024,
    defaultHeight: 768
  })

  // Create the browser window.
  mainWindow = new BrowserWindow({
    x: mainWindowState.x,
    y: mainWindowState.y,
    width: mainWindowState.width,
    height: mainWindowState.height,
    show: false,
    autoHideMenuBar: true,
    webPreferences: {
      preload: join(__dirname, '../preload/index.cjs'),
      sandbox: false,
      webSecurity: false
    }
  })
  mainWindowState.manage(mainWindow)


  const old_menu = Menu.getApplicationMenu()
  const new_menu = old_menu?.items.filter((item) => item.role !== 'help')
  Menu.setApplicationMenu(Menu.buildFromTemplate(new_menu))
  mainWindow.setAutoHideMenuBar(true)
  mainWindow.menuBarVisible = false


  switch (process.platform) {
    case 'win32':
      mainWindow.setIcon(
        join(VITE_PUBLIC, 'favicon.ico')
      )
      break
    default:
      break
  }



  //open the pages different from the Kitsu server in the default browser
  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    try {
      const url_server = new URL(store.get('login.server'))
      if (
        url.startsWith(String(url_server)) &&
        store.get('login.access_token') !== ''
      ) {
        return { action: 'allow' }
      }
    } catch (error) {
      // do nothing
    }
    open(url)
    return { action: 'deny' }
  })

  /**
   * If you install `show: true` then it can cause issues when trying to close the window.
   * Use `show: false` and listener events `ready-to-show` to fix these issues.
   *
   * @see https://github.com/electron/electron/issues/25012
   */
  mainWindow.on('ready-to-show', () => {
    mainWindow.show()
    if (isDevelopment) {
      mainWindow.webContents.openDevTools()
    }
  })


  // HMR for renderer base on electron-vite cli.
  // Load the remote URL for development or the local html file for production.
  if (is.dev && process.env['ELECTRON_RENDERER_URL']) {
    mainWindow.loadURL(process.env['ELECTRON_RENDERER_URL'])
  } else {
    mainWindow.loadFile(join(__dirname, '../renderer/index.html'))
  }


  var codePage = undefined
  if (process.platform === 'win32') {
    exec('chcp', (err, stdout, stderr) => {
      if (stdout) {
        try {
          codePage = Number(stdout.split(':')[1])
        } catch {
          codePage = undefined
        }
      }
    })
  }


  ipcMain.handle('dark-theme:toggle', () => {
    nativeTheme.themeSource = store.get('main.isDarkTheme') ? 'dark' : 'light'
    return store.get('main.isDarkTheme')
  })


  ipcMain.handle('launch-command:post-exports', (event, command, variables) => {
    if (command === '') {
      console.log('No command to launch before importing to Kitsu Publisher.')
      return false
    } else {
      command = formatUnicorn(command, variables)
      const commandOutput = { output: '', command: command, statusCode: 0 }
      const commandSpawn = spawn(command, [], {
        shell: true,
        // encoding: 'buffer',
        windowsHide: true,
        env: { ...process.env, ...variables },
        timeout: 60000 // TODO : make the timeout configurable
      })
      console.log(
        `Launch command "${command}" before importing to Kitsu Publisher.`
      )

      const manageOutputData = (data, isStdout) => {
        var output
        if (process.platform === 'win32' && codePage !== undefined) {
          // get Windows code page
          output = iconv.decode(data, `cp${codePage}`)
        } else {
          output = iconv.decode(data, 'utf8')
        }
        commandOutput.output += isStdout ? output : colors.red(output)
      }

      commandSpawn.stdout.on('data', (data) => {
        manageOutputData(data, true)
      })

      commandSpawn.stderr.on('data', (data) => {
        manageOutputData(data, false)
      })

      commandSpawn.on('close', (statusCode) => {
        console.log('Output :')
        console.log(commandOutput.output)
        console.log(`Command exited with status code ${statusCode}.`)
        commandOutput.statusCode = statusCode
        mainWindow.webContents.send('commandOutput', commandOutput)
      })

      return true
    }
  })

  ipcMain.handle('open-dialog:show', (event, options) => {
    return dialog.showOpenDialogSync(mainWindow, options)
  })

}

app.on('second-instance', () => {
  // Someone tried to run a second instance, we should focus our window.
  if (mainWindow) {
    if (mainWindow.isMinimized()) mainWindow.restore()
    mainWindow.focus()
  }
})

app.on('certificate-error', (event, webContents, url, error, certificate, callback) => {
  // On certificate error we disable default behaviour (stop loading the page)
  // and we then say "it is all fine - true" to the callback
  event.preventDefault();
  callback(true);
});


// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(createWindow)


// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

// In this file you can include the rest of your app"s specific main process
// code. You can also put them in separate files and require them here.

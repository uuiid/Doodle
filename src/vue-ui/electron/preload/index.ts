import { ipcRenderer, contextBridge } from 'electron'
import { electronAPI } from '@electron-toolkit/preload'
import { readFileSync, access, accessSync, constants, PathLike, readdirSync } from 'fs'
import { store, config } from '../main/store'
import path from 'path'
const io = require('socket.io-client')

// --------- Expose some API to the Renderer process ---------
contextBridge.exposeInMainWorld('ipcRenderer', {
  on(...args: Parameters<typeof ipcRenderer.on>) {
    const [channel, listener] = args
    return ipcRenderer.on(channel, (event, ...args) => listener(event, ...args))
  },
  off(...args: Parameters<typeof ipcRenderer.off>) {
    const [channel, ...omit] = args
    return ipcRenderer.off(channel, ...omit)
  },
  send(...args: Parameters<typeof ipcRenderer.send>) {
    const [channel, ...omit] = args
    return ipcRenderer.send(channel, ...omit)
  },
  invoke(...args: Parameters<typeof ipcRenderer.invoke>) {
    const [channel, ...omit] = args
    return ipcRenderer.invoke(channel, ...omit)
  },
  ...electronAPI
  // You can expose other APTs you need here.
  // ...
})


let socketio = null

const apiKey = 'electron'
/**
 * @see https://github.com/electron/electron/issues/21437#issuecomment-573522360
 */
const api = {
  store: {
    get: (key) => {
      return store.get(key)
    },
    set: (key, value) => {
      return store.set(key, value)
    },
    delete: (key) => {
      return store.delete(key)
    }
  },
  config: {
    get: (key) => {
      return config.get(key)
    },
    set: (key, value) => {
      return config.set(key, value)
    },
    delete: (key) => {
      return config.delete(key)
    }
  },
  file: {
    readFileSync: (filepath) => {
      return readFileSync(filepath)
    }
  },
  openDialog: (options) => {
    return ipcRenderer.invoke('open-dialog:show', options)
  },
  fileExists: (in_path: PathLike, callback: (arg0: boolean) => void ) => {
    access(path.normalize(in_path.toString()), constants.F_OK, (err)=>{
      if (err){
        console.log(err)
        callback(false)
      }
      callback(true)
    })
  },
  fileExistsSync(in_path: PathLike): boolean {
    try {
      accessSync(path.normalize(in_path.toString()), constants.F_OK)
      return true
    } catch (err) {
      console.log(err)
      return false
    }
  },
  filereaddirSync(in_path: PathLike) {
    return readdirSync(path.normalize(in_path.toString())); 
  },
  filefilestem(in_path: PathLike) {
    return path.parse(in_path.toString()).name
  },
  launchCommandBeforeExport: (command, variables) => {
    return ipcRenderer.invoke('launch-command:post-exports', command, variables)
  },
  toggleDarkTheme: () => {
    return ipcRenderer.invoke('dark-theme:toggle')
  },
  socketio: {
    create: () => {
      socketio = io(`${store.get('login.server')}/events`, {
        transportOptions: {
          polling: {
            extraHeaders: {
              Authorization: `Bearer ${store.get('login.access_token')}`,
              'User-Agent': `Kitsu publisher ${config.get('appVersion')}`
            }
          }
        }
      })
    },
    destroy: () => {
      if (socketio !== null) {
        socketio.disconnect()
      }
      socketio = null
    },
    on: (event, fun) => {
      if (socketio !== null) {
        socketio.on(event, fun)
      }
    },
    off: (event, fun) => {
      if (socketio !== null) {
        socketio.off(event, fun)
      }
    },
    connect: () => {
      if (socketio !== null) {
        socketio.connect()
      }
    },
    disconnect: () => {
      if (socketio !== null) {
        socketio.disconnect()
      }
    }
  },
  ipcRenderer: {
    on: (channel, listener) => {
      ipcRenderer.on(channel, listener)
    },
    removeListener: (channel, listener) => {
      ipcRenderer.removeListener(channel, listener)
    },
    removeAllListeners: (channel) => {
      ipcRenderer.removeAllListeners(channel)
    }
  }
}

/**
 * The "Main World" is the JavaScript context that your main renderer code runs in.
 * By default, the page you load in your renderer executes code in this world.
 *
 * @see https://www.electronjs.org/docs/api/context-bridge
 */
contextBridge.exposeInMainWorld(apiKey, api)

// --------- Preload scripts loading ---------
function domReady(condition: DocumentReadyState[] = ['complete', 'interactive']) {
  return new Promise((resolve) => {
    if (condition.includes(document.readyState)) {
      resolve(true)
    } else {
      document.addEventListener('readystatechange', () => {
        if (condition.includes(document.readyState)) {
          resolve(true)
        }
      })
    }
  })
}

const safeDOM = {
  append(parent: HTMLElement, child: HTMLElement) {
    if (!Array.from(parent.children).find(e => e === child)) {
      return parent.appendChild(child)
    }
  },
  remove(parent: HTMLElement, child: HTMLElement) {
    if (Array.from(parent.children).find(e => e === child)) {
      return parent.removeChild(child)
    }
  },
}

// /**
//  * https://tobiasahlin.com/spinkit
//  * https://connoratherton.com/loaders
//  * https://projects.lukehaas.me/css-loaders
//  * https://matejkustec.github.io/SpinThatShit
//  */
// function useLoading() {
//   const className = `loaders-css__square-spin`
//   const styleContent = `
// @keyframes square-spin {
//   25% { transform: perspective(100px) rotateX(180deg) rotateY(0); }
//   50% { transform: perspective(100px) rotateX(180deg) rotateY(180deg); }
//   75% { transform: perspective(100px) rotateX(0) rotateY(180deg); }
//   100% { transform: perspective(100px) rotateX(0) rotateY(0); }
// }
// .${className} > div {
//   animation-fill-mode: both;
//   width: 50px;
//   height: 50px;
//   background: #fff;
//   animation: square-spin 3s 0s cubic-bezier(0.09, 0.57, 0.49, 0.9) infinite;
// }
// .app-loading-wrap {
//   position: fixed;
//   top: 0;
//   left: 0;
//   width: 100vw;
//   height: 100vh;
//   display: flex;
//   align-items: center;
//   justify-content: center;
//   background: #282c34;
//   z-index: 9;
// }
//     `
//   const oStyle = document.createElement('style')
//   const oDiv = document.createElement('div')

//   oStyle.id = 'app-loading-style'
//   oStyle.innerHTML = styleContent
//   oDiv.className = 'app-loading-wrap'
//   oDiv.innerHTML = `<div class="${className}"><div></div></div>`

//   return {
//     appendLoading() {
//       safeDOM.append(document.head, oStyle)
//       safeDOM.append(document.body, oDiv)
//     },
//     removeLoading() {
//       safeDOM.remove(document.head, oStyle)
//       safeDOM.remove(document.body, oDiv)
//     },
//   }
// }

// // ----------------------------------------------------------------------

// const { appendLoading, removeLoading } = useLoading()
// domReady().then(appendLoading)

// window.onmessage = (ev) => {
//   ev.data.payload === 'removeLoading' && removeLoading()
// }

// setTimeout(removeLoading, 4999)

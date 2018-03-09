import sys
import m3u8
import aiohttp
import asyncio
import threading
import subprocess

from traceback import print_exc

from win32gui import EnumWindows, GetWindowText

from PyQt5.QtWidgets import QApplication, QWidget, QSplitter, QMainWindow, QAction
from PyQt5.QtGui import QWindow, QKeySequence, QPalette, QIcon
from PyQt5.QtCore import Qt, QTimer

ICON_PATH = r'C:\Users\depar\Temporary\twitch-vlc\icon.png'

META_TITLE = lambda channel_name: f'Twitch-VLC-{channel_name}'

VLC_PATH = r'C:\Program Files (x86)\VideoLAN\VLC\vlc.exe'
VLC_ARGS = lambda title: (
    '--qt-minimal-view', '--no-qt-video-autoresize', f'--meta-title={title}',
    '--key-faster=ctrl+f', '--key-rate-normal=ctrl+n',
    '--network-caching=3000', '--live-caching=3000'
)

CHROME_PATH = r'C:\Program Files (x86)\Google\Chrome\Application\chrome.exe'
CHROME_ARGS = lambda channel_name: (
    f'--app=https://www.twitch.tv/popout/{channel_name}/chat?popout=',
)

class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setup_menus()
        self.splitter = QSplitter(self)
        self.setCentralWidget(self.splitter)

        palette = QPalette()
        palette.setColor(QPalette.Background, Qt.black)
        self.setPalette(palette)
        self.setAutoFillBackground(True)

    def setup_menus(self):
        menu_bar = self.menuBar()

        view_menu = menu_bar.addMenu("View")

        fullscreen_action = QAction("FullScreen", self)
        fullscreen_action.setCheckable(True)
        fullscreen_action.toggled.connect(self.fullscreen)

        view_menu.addAction(fullscreen_action)

    def fullscreen(self, on):
        state = self.windowState()
        if on:
            state |= Qt.WindowFullScreen
        else:
            state &= ~Qt.WindowFullScreen
        self.setWindowState(state)
        self.menuBar().setVisible(not on)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_F11:
            self.fullscreen(not self.isFullScreen())

    def add_windows(self, *handles):
        for handle in handles:
            as_window = QWindow.fromWinId(handle)
            as_widget = QWidget.createWindowContainer(as_window, self)
            self.splitter.addWidget(as_widget)

def get_handle_by(pred):
    win_handle = None

    def on_win(handle, _):
        nonlocal win_handle
        if pred(GetWindowText(handle)):
            win_handle = handle
        return True

    EnumWindows(on_win, None)
    return win_handle

async def playlist_url(session, channel_name):
    url = f'https://streamer.datcoloc.com/{channel_name}'
    async with session.get(url) as response:
        data = await response.json()
        return data['playlists'][0]['url']

class SegmentIterator:
    def __init__(self, session, url):
        self.url = url
        self.session = session
        self.buffer = asyncio.Queue()
        asyncio.ensure_future(self.fetch())

    async def fetch(self):
        last_segment_uri = None
        while True:
            async with self.session.get(self.url) as response:
                playlist = m3u8.loads(await response.text())
                if last_segment_uri is None:
                    segments = playlist.segments[-1:]
                else:
                    segment_index = next(
                        i for (i, segment) in enumerate(playlist.segments)
                        if segment.uri == last_segment_uri
                    )
                    segments = playlist.segments[segment_index+1:]
                if segments:
                    last_segment_uri = segments[-1].uri
                    segments_data = await asyncio.gather(
                        *(self.fetch_segment(segment) for segment in segments)
                    )
                    for segment_data in segments_data:
                        await self.buffer.put(segment_data)
                else:
                    await asyncio.sleep(.5)

    async def fetch_segment(self, segment):
        async with self.session.get(segment.uri) as response:
            return await response.read()

    async def __aiter__(self):
        return self

    async def __anext__(self):
        item = await self.buffer.get()
        return item

async def run_stream(session, url, writer):
    async for segment_data in SegmentIterator(session, url):
        try:
            writer.write(segment_data)
        except Exception as e:
            print(f'error writing data: {e}')
            break

async def place_windows(meta_title):
    get_vlc_handle = lambda: get_handle_by(lambda t: t.startswith(meta_title))
    get_chat_handle = lambda: get_handle_by(lambda t: t.endswith('Twitch'))

    while True:
        vlc_handle, chat_handle = get_vlc_handle(), get_chat_handle()
        if vlc_handle and chat_handle:
            th = threading.Thread(target=gui_main, args=(vlc_handle, chat_handle, meta_title))
            th.start()
        await asyncio.sleep(.2)

async def fetcher_logic(channel_name):
    meta_title = META_TITLE(channel_name)

    vlc_process = subprocess.Popen(
        [VLC_PATH, '-', *VLC_ARGS(meta_title)],
        stdin=subprocess.PIPE
    )
    subprocess.Popen([CHROME_PATH, *CHROME_ARGS(channel_name)])

    async with aiohttp.ClientSession() as session:
        url = await playlist_url(session, channel_name)
        await asyncio.gather(
            run_stream(session, url, vlc_process.stdin),
            place_windows(meta_title)
        )

def gui_main(vlc_handle, chat_handle, title):
    app = QApplication(sys.argv)

    main_window = MainWindow()
    main_window.setWindowTitle(f'Globi_{title}')
    main_window.setWindowIcon(QIcon(ICON_PATH))
    main_window.resize(1280, 720)

    main_window.add_windows(vlc_handle, chat_handle)

    main_window.showMaximized()

    app.exec_()


def main():
    channel_name = sys.argv[1].replace('twitch-player://', '').replace('/', '').lower()
    try:
        loop = asyncio.ProactorEventLoop()
        asyncio.set_event_loop(loop)
        loop.run_until_complete(fetcher_logic(channel_name))
    except Exception:
        print_exc()

if __name__ == '__main__':
    main()

chrome.browserAction.onClicked.addListener(function(tab) {
  const url = new URL(tab.url),
        channel = url.pathname.slice(1);

  if (url.host == 'www.twitch.tv' && !channel.includes('/')) {
    chrome.tabs.executeScript({
      code: `window.location="twitch-player://${channel}"`
    });
    setTimeout(() => chrome.tabs.remove(tab.id, () => {}), 3000)
  }

});

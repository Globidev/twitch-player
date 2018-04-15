extern crate hyper;
extern crate futures;
extern crate tokio_timer;

use self::futures::{future, Future, Stream};
use self::futures::sync::mpsc;
use self::hyper::Chunk;

use self::tokio_timer::Timer;

use twitch::types::PlaylistInfo;

use std::rc::Rc;
use std::cell::RefCell;

pub type PlayerSink = mpsc::Sender<Result<hyper::Chunk, hyper::Error>>;

pub struct StreamPlayer {
    playlist_info: PlaylistInfo,
    sinks: Rc<RefCell<Vec<PlayerSink>>>,
}

impl StreamPlayer {
    pub fn new(playlist_info: PlaylistInfo) -> Self {
        Self {
            playlist_info: playlist_info,
            sinks: Rc::new(RefCell::new(Vec::new())),
        }
    }

    pub fn play(&self) -> impl Future<Item = (), Error = StreamPlayerError> {
        let write_stuff = {
            let sinks = Rc::clone(&self.sinks);
            let mut counter = 0;

            move |_| {
                for sink in sinks.borrow_mut().iter_mut() {
                    let data = format!("Imagine some video chunk: {}\r\n", counter);

                    sink.try_send(Ok(Chunk::from(data)))
                        .unwrap_or_default();
                }
                counter += 1;
                Ok(())
            }
        };

        Timer::default()
            .interval(::std::time::Duration::from_millis(250))
            .for_each(write_stuff)
            .map(|_| { })
            .map_err(|_| StreamPlayerError)
    }

    pub fn add_sink(&self, sink: PlayerSink) {
        self.sinks.borrow_mut().push(sink)
    }
}

pub struct StreamPlayerError;

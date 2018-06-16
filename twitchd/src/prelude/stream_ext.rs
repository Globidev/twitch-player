extern crate tokio;
extern crate bytes;

use self::tokio::timer;
use self::bytes::Bytes;
use super::futures::*;

use std::time::{Duration, Instant};

pub trait StreamExtTimeout {
    fn timeout(self, duration: Duration) -> TimeoutStream<Self>
    where
        Self: Stream + Sized
    {
        TimeoutStream::new(self, duration)
    }
}

impl<S: Stream> StreamExtTimeout for S { }

pub trait StreamExtBufferConcat {
    fn buffer_concat(self, buf_size: usize) -> BufferConcat<Self>
        where
            Self: Stream + Sized,
            Self::Item: AsRef<[u8]>
    {
        BufferConcat::new(self, buf_size)
    }
}

impl<S> StreamExtBufferConcat for S
where
    S: Stream,
    S::Item: AsRef<[u8]>
{ }

pub struct TimeoutStream<S> {
    stream: S,
    duration: Duration,
    delay_timer: timer::Delay,
}

pub struct BufferConcat<S>
where
    S: Stream,
    S::Item: AsRef<[u8]>,
{
    stream: S,
    buf_size: usize,
    buffered: Bytes,
    err: Option<S::Error>,
}

impl<S: Stream> TimeoutStream<S> {
    fn new(stream: S, duration: Duration) -> Self {
        let delay_timer = timer::Delay::new(Instant::now() + duration);

        Self { stream, duration, delay_timer }
    }
}

impl<S> BufferConcat<S>
where
    S: Stream,
    S::Item: AsRef<[u8]>,
{
    fn new(stream: S, buf_size: usize) -> BufferConcat<S> {
        let buffered = Bytes::with_capacity(buf_size);
        let err = None;

        Self { stream, buf_size, buffered, err }
    }

    fn take_buffer(&mut self) -> Bytes {
        use std::mem;
        mem::replace(&mut self.buffered, Bytes::with_capacity(self.buf_size))
    }
}

impl<S> Stream for TimeoutStream<S>
where
    S: Stream,
    S::Error: From<TimeoutError>,
{
    type Item = S::Item;
    type Error = S::Error;

    fn poll(&mut self) -> Poll<Option<Self::Item>, Self::Error> {
        match self.stream.poll() {
            // If an error occured with the stream, we error with it
            err @ Err(_) => err,
            // If an item is ready, reset the timeout before returning the item
            item @ Ok(Async::Ready(_)) => {
                self.delay_timer.reset(Instant::now() + self.duration);
                item
            }
            // If the stream is not ready, proceed to polling the timer
            Ok(Async::NotReady) => match self.delay_timer.poll() {
                // If the timer errored, return its error
                Err(timer_error) => Err(TimeoutError::Timer(timer_error).into()),
                // If the timer did not timeout yet then the stream is not ready
                Ok(Async::NotReady) => Ok(Async::NotReady),
                // If the timer timedout, return a timeout error
                Ok(Async::Ready(_)) => Err(TimeoutError::TimedOut.into())
            }
        }
    }
}

impl<S> Stream for BufferConcat<S>
where
    S: Stream,
    S::Item: AsRef<[u8]>,
{
    type Item = bytes::Bytes;
    type Error = S::Error;

    fn poll(&mut self) -> Poll<Option<Self::Item>, Self::Error> {
        // Abort right away if we encountered an error during the last poll
        if let Some(err) = self.err.take() {
            return Err(err)
        }
        // Here we are gonna poll the inner stream in a loop because we might
        // be able to buffer multiple consecutive ready items
        loop {
            match self.stream.poll() {
                // If the inner stream is not ready then we wait
                Ok(Async::NotReady) => return Ok(Async::NotReady),
                // In case the inner stream errored
                Err(error) => return match self.buffered.is_empty() {
                    // If the buffer is empty we can abort with the error right
                    // away
                    true  => Err(error),
                    // If we still have buffered items, we first need to yield
                    // them back before erroring at the next call to poll
                    false => {
                        self.err = Some(error);
                        Ok(Async::Ready(Some(self.take_buffer())))
                    }
                },
                // If the inner stream has some item ready, we can buffer it and
                // maybe yield the buffer back if we reached the capacity
                Ok(Async::Ready(Some(data))) => {
                    self.buffered.extend_from_slice(data.as_ref());

                    if self.buffered.len() >= self.buf_size {
                        return Ok(Async::Ready(Some(self.take_buffer())))
                    }
                }
                // In case the stream ended
                Ok(Async::Ready(None)) => return match self.buffered.is_empty() {
                    // If the buffer is empty, we can end right away
                    true  => Ok(Async::Ready(None)),
                    // Otherwise we need to yield our buffer before ending at
                    // the next call to poll
                    false => Ok(Async::Ready(Some(self.take_buffer())))
                }
            }
        }

    }
}

pub enum TimeoutError {
    TimedOut,
    Timer(timer::Error),
}

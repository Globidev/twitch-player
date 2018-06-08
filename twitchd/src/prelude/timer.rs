extern crate tokio;

use super::futures::*;

use std::time::{Duration, Instant};

pub fn timeout_stream<S>(stream: S, duration: Duration) -> TimeoutStream<S>
where
    S: Stream
{
    let delay_timer = tokio::timer::Delay::new(Instant::now() + duration);

    TimeoutStream {
        stream,
        duration,
        delay_timer
    }
}

pub enum TimeoutError {
    TimedOut,
    Timer(tokio::timer::Error),
}

pub struct TimeoutStream<S> {
    stream: S,
    duration: Duration,
    delay_timer: tokio::timer::Delay,
}

impl<T, E> Stream for TimeoutStream<T>
where
    T: Stream<Error = E>,
    E: From<TimeoutError>,
{
    type Item = T::Item;
    type Error = E;

    fn poll(&mut self) -> Poll<Option<T::Item>, E> {
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
                Err(timer_error) => Err(E::from(TimeoutError::Timer(timer_error))),
                // If the timer did not timeout yet then the stream is not ready
                Ok(Async::NotReady) => Ok(Async::NotReady),
                // If the timer timedout, return a timeout error
                Ok(Async::Ready(_)) => Err(E::from(TimeoutError::TimedOut))
            }
        }
    }
}

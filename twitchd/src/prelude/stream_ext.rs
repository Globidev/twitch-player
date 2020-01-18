use futures::prelude::*;
use std::pin::Pin;
use std::task::{Context, Poll};
use pin_project::pin_project;

pub trait StreamExtChunkConcat {
    fn chunk_concat<R>(self, chunk_len: usize) -> ChunkConcat<Self, R>
    where
        Self: Sized;
}

impl<S: Stream> StreamExtChunkConcat for S {
    fn chunk_concat<R>(self, chunk_len: usize) -> ChunkConcat<Self, R> {
        ChunkConcat {
            stream: self.fuse(),
            chunk_len,
            buffer: Vec::with_capacity(chunk_len),
        }
    }
}

#[pin_project]
pub struct ChunkConcat<S, R> {
    #[pin] stream: stream::Fuse<S>,
    chunk_len: usize,
    buffer: Vec<R>,
}

impl<S, T, R, E> Stream for ChunkConcat<S, R>
where
    S: Stream<Item = Result<T, E>>,
    T: AsRef<[R]>,
    R: Clone
{
    type Item = Result<Vec<R>, E>;

    fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        match self.as_mut().project() {
            this => if this.buffer.len() >= *this.chunk_len {
                let rest = this.buffer.split_off(*this.chunk_len);
                return Poll::Ready(Some(Ok(std::mem::replace(this.buffer, rest))))
            }
        }

        loop {
            let this = self.as_mut().project();

            match this.stream.poll_next(cx) {
                Poll::Pending => return Poll::Pending,
                Poll::Ready(None) => break,
                Poll::Ready(Some(Err(e))) => return Poll::Ready(Some(Err(e))),
                Poll::Ready(Some(Ok(data))) => {
                    let bytes = data.as_ref();
                    if this.buffer.len() + bytes.len() > *this.chunk_len {
                        let max_amount = *this.chunk_len - this.buffer.len();
                        let (left, right) = bytes.split_at(max_amount);
                        this.buffer.extend_from_slice(left);
                        let mut next_buffer = Vec::with_capacity(*this.chunk_len);
                        next_buffer.extend_from_slice(right);
                        return Poll::Ready(Some(Ok(std::mem::replace(this.buffer, next_buffer))))
                    } else {
                        this.buffer.extend_from_slice(bytes)
                    }
                },
            }
        }

        let this = self.as_mut().project();

        if this.buffer.is_empty() {
            Poll::Ready(None)
        } else {
            Poll::Ready(Some(Ok(std::mem::take(this.buffer))))
        }
    }
}

#[cfg(test)]
mod tests {
    async fn chunk_concat_ok<T, A>(it: impl IntoIterator<Item = A>, chunk_len: usize) -> Vec<Vec<T>>
    where
        A: AsRef<[T]>,
        T: Clone,
    {
        use super::StreamExtChunkConcat;
        use std::convert::Infallible;
        use futures::prelude::*;

        stream::iter(it).map(Ok::<_, Infallible>)
            .chunk_concat(chunk_len)
            .try_collect()
            .await
            .unwrap()
    }

    #[tokio::test]
    async fn test_chunk_concat() {
        assert_eq!(
            chunk_concat_ok(vec!["Hello", "", ", ", "World!"], 4).await,
            vec![b"Hell".to_vec(), b"o, W".to_vec(), b"orld".to_vec(), b"!".to_vec()]
        );

        assert_eq!(
            chunk_concat_ok(vec![[1,2,3], [4,5,6], [7,8,9]], 5).await,
            vec![vec![1,2,3,4,5], vec![6,7,8,9]]
        );

        assert_eq!(
            chunk_concat_ok(vec![[1,2,3], [4,5,6], [7,8,9]], 10).await,
            vec![vec![1,2,3,4,5,6,7,8,9]]
        );

        assert_eq!(
            chunk_concat_ok(vec![[1,2,3], [4,5,6], [7,8,9]], 2).await,
            vec![vec![1,2], vec![3,4], vec![5,6], vec![7,8], vec![9]]
        );

        assert_eq!(
            chunk_concat_ok(vec![[1,2,3,4,5,6]], 2).await,
            vec![vec![1,2], vec![3,4], vec![5,6]]
        );

        assert_eq!(
            chunk_concat_ok(vec![[1,2,3,4,5,6]], 10).await,
            vec![vec![1,2,3,4,5,6]]
        );
    }
}


#![allow(clippy::missing_safety_doc)]

use tokio::{runtime, sync::oneshot};

pub struct Aio {
    rt_handle: runtime::Handle,
    rt_thread: std::thread::JoinHandle<Result<(), oneshot::error::RecvError>>,
    shutdown_tx: oneshot::Sender<()>,
}

#[no_mangle]
pub extern "C" fn aio__construct() -> *const Aio {
    println!("aio__construct");

    let rt = match runtime::Builder::new_current_thread().enable_time().build() {
        Ok(rt) => rt,
        Err(e) => {
            eprintln!("error constructing aio instance: {e}");
            return std::ptr::null();
        }
    };
    let rt_handle = rt.handle().clone();

    let (shutdown_tx, shutdown_rx) = oneshot::channel();
    let rt_thread = std::thread::spawn(move || {
        rt.block_on(async { shutdown_rx.await })?;
        rt.shutdown_timeout(std::time::Duration::from_millis(500));
        println!("Runtime shutdown");
        Ok(())
    });

    Box::into_raw(Box::new(Aio {
        rt_handle,
        rt_thread,
        shutdown_tx,
    }))
}

#[no_mangle]
pub unsafe extern "C" fn aio__destruct(ptr: *mut Aio) {
    if !ptr.is_null() {
        let aio = Box::from_raw(ptr);
        drop(aio.rt_handle);
        if aio.shutdown_tx.send(()).is_err() {
            eprintln!("error sending shutdown mesg");
        }
        if let Err(e) = aio.rt_thread.join() {
            eprintln!("error joining rt thread: {e:?}");
        }
        println!("aio__destruct");
    }
}

// Note: Rust can't track the lifetime of raw pointers and thus won't let us pass
// it across threads here (not Send), so we cheat and receive it as a usize.
#[no_mangle]
pub unsafe extern "C" fn aio__sleep(
    ptr: *const Aio,
    ms: u32,
    ctx: usize,
    cb: Option<extern "C" fn(usize)>,
) {
    if !ptr.is_null() {
        if let Some(cb) = cb {
            let aio = &*ptr;
            let _guard = aio.rt_handle.enter();
            tokio::task::spawn(async move {
                tokio::time::sleep(std::time::Duration::from_millis(u64::from(ms))).await;
                cb(ctx);
            });
        }
    }
}

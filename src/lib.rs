#![allow(clippy::missing_safety_doc)]

pub struct Aio {}

#[no_mangle]
pub extern "C" fn aio__construct() -> *const Aio {
    println!("aio__construct");
    Box::into_raw(Box::new(Aio {}))
}

#[no_mangle]
pub unsafe extern "C" fn aio__destruct(ptr: *mut Aio) {
    drop(Box::from_raw(ptr));
    println!("aio__destruct");
}

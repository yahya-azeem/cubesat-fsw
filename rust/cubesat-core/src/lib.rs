//! Hardware-independent mission logic for the 1U CubeSat.
//!
//! Deliberately `no_std`, allocation-free and panic-free on the happy path so
//! the exact same code runs in host unit tests and on the VA41620.
//!
//! Layout mirrors the design document:
//! - [`power`]  energy budget / battery guard (the 1 W-class EPS)
//! - [`fsm`]    duty-cycled store-and-forward mode machine
//! - [`frames`] CCSDS TM transfer-frame primary header

#![cfg_attr(not(test), no_std)]
#![forbid(unsafe_code)]

pub mod frames;
pub mod fsm;
pub mod power;

pub use fsm::{Event, Mode, Supervisor};
pub use power::{mwh, Battery, PowerError};

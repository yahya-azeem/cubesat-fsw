//! Duty-cycled store-and-forward supervisor.
//!
//! The spacecraft spends most of the orbit in [`Mode::Charge`]; the S-band
//! transmitter is only reachable through [`Mode::Downlink`], which requires a
//! predicted pass *and* enough energy in the reserve-guarded battery.

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Mode {
    /// Fault or boot: payload and radios off, only the beacon may run.
    Safe = 0,
    /// Sunlit eclipse-free cruise: harvest, payload off, UHF listening.
    Charge = 1,
    /// Pass is close: ADCS active, S-band warm-up permitted.
    AwaitPass = 2,
    /// Burst downlink window; the only mode that may enable the PA.
    Downlink = 3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Event {
    /// POST passed, battery above the reserve floor.
    FaultCleared,
    /// A ground pass has entered the scheduling horizon.
    PassScheduled,
    /// The pass was cancelled, missed, or the geometry degraded.
    PassCancelled,
    /// AOS: the ground station is acquiring the downlink.
    PassAcquired,
    /// All queued telemetry/payload has been flushed, or LOS.
    DownlinkComplete,
    /// Any guard tripped (battery, thermal, watchdog).
    Fault,
    /// Housekeeping watchdog tick; no state change, used for liveness.
    Tick,
}

#[derive(Debug, Clone, Copy)]
pub struct Supervisor {
    mode: Mode,
}

impl Supervisor {
    /// Always boots into `Safe`; nothing is trusted before POST.
    pub const fn new() -> Self {
        Self { mode: Mode::Safe }
    }

    pub const fn mode(&self) -> Mode {
        self.mode
    }

    /// True only while the high-power transmitter may be keyed.
    pub const fn pa_permitted(&self) -> bool {
        matches!(self.mode, Mode::Downlink)
    }

    /// Consume one event and return the resulting mode.
    ///
    /// Invalid transitions are ignored (fail-closed), never panicking.
    pub fn step(&mut self, event: Event) -> Mode {
        use Event::*;
        use Mode::*;

        self.mode = match (self.mode, event) {
            // Faults always win.
            (_, Fault) => Safe,

            (Safe, FaultCleared) => Charge,

            (Charge, PassScheduled) => AwaitPass,
            (Charge, DownlinkComplete | PassAcquired) => Charge,

            (AwaitPass, PassAcquired) => Downlink,
            (AwaitPass, PassCancelled) => Charge,

            (Downlink, DownlinkComplete) => Charge,
            (Downlink, PassCancelled) => AwaitPass,

            // Everything else, including redundant events, holds state.
            (mode, _) => mode,
        };
        self.mode
    }
}

impl Default for Supervisor {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn boots_safe_and_requires_post() {
        let mut s = Supervisor::new();
        assert_eq!(s.mode(), Mode::Safe);
        assert!(!s.pa_permitted());
        s.step(Event::Tick);
        assert_eq!(s.mode(), Mode::Safe);
        s.step(Event::FaultCleared);
        assert_eq!(s.mode(), Mode::Charge);
    }

    #[test]
    fn happy_path_pass() {
        let mut s = Supervisor::new();
        for ev in [
            Event::FaultCleared,
            Event::PassScheduled,
            Event::PassAcquired,
            Event::DownlinkComplete,
        ] {
            s.step(ev);
        }
        assert_eq!(s.mode(), Mode::Charge);
    }

    #[test]
    fn pa_only_while_downlinking() {
        let mut s = Supervisor::new();
        s.step(Event::FaultCleared);
        s.step(Event::PassScheduled);
        assert!(!s.pa_permitted());
        s.step(Event::PassAcquired);
        assert!(s.pa_permitted());
        s.step(Event::Fault);
        assert!(!s.pa_permitted());
    }

    #[test]
    fn fault_is_sticky_until_cleared() {
        let mut s = Supervisor::new();
        s.step(Event::FaultCleared);
        s.step(Event::PassScheduled);
        s.step(Event::PassAcquired);
        s.step(Event::Fault);
        assert_eq!(s.mode(), Mode::Safe);
        s.step(Event::PassAcquired);
        assert_eq!(s.mode(), Mode::Safe);
        s.step(Event::FaultCleared);
        assert_eq!(s.mode(), Mode::Charge);
    }

    #[test]
    fn bypassing_aos_does_nothing() {
        let mut s = Supervisor::new();
        s.step(Event::FaultCleared);
        s.step(Event::PassAcquired); // no scheduled pass
        assert_eq!(s.mode(), Mode::Charge);
        assert!(!s.pa_permitted());
    }

    #[test]
    fn cancelled_pass_returns_to_charge() {
        let mut s = Supervisor::new();
        s.step(Event::FaultCleared);
        s.step(Event::PassScheduled);
        s.step(Event::PassCancelled);
        assert_eq!(s.mode(), Mode::Charge);
    }
}

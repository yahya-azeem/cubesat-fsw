//! Energy accounting for the store-and-forward EPS.

/// Convert a power draw (milliwatts) held for `seconds` into milliwatt-hours.
///
/// Integer-only so it behaves identically on any target; truncated to 1 mWh.
pub const fn mwh(power_mw: i64, seconds: i64) -> i64 {
    power_mw * seconds / 3600
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PowerError {
    /// Discharge would eat into the protected reserve.
    InsufficientEnergy,
    /// Negative power / time requested.
    InvalidArgument,
}

/// Fraction of capacity that may never be spent, in permille.
/// 200 permille = 20 % depth-of-discharge floor; raise for cold orbits.
pub const DEFAULT_RESERVE_PERMILLE: i32 = 200;

#[derive(Debug, Clone, Copy)]
pub struct Battery {
    capacity_mwh: i64,
    stored_mwh: i64,
    reserve_permille: i32,
}

impl Battery {
    pub const fn new(capacity_mwh: i64) -> Self {
        Self {
            capacity_mwh,
            stored_mwh: 0,
            reserve_permille: DEFAULT_RESERVE_PERMILLE,
        }
    }

    pub const fn from_soc(capacity_mwh: i64, soc_permille: i32) -> Self {
        let p = if soc_permille < 0 {
            0
        } else if soc_permille > 1000 {
            1000
        } else {
            soc_permille
        };
        Self {
            capacity_mwh,
            stored_mwh: capacity_mwh * p as i64 / 1000,
            reserve_permille: DEFAULT_RESERVE_PERMILLE,
        }
    }

    pub const fn capacity_mwh(&self) -> i64 {
        self.capacity_mwh
    }

    pub const fn stored_mwh(&self) -> i64 {
        self.stored_mwh
    }

    pub const fn reserve_mwh(&self) -> i64 {
        self.capacity_mwh * self.reserve_permille as i64 / 1000
    }

    pub fn soc_permille(&self) -> i32 {
        if self.capacity_mwh <= 0 {
            return 0;
        }
        ((self.stored_mwh * 1000) / self.capacity_mwh) as i32
    }

    /// Add energy from the solar array. Returns the amount actually accepted.
    pub fn charge(&mut self, power_mw: i64, seconds: i64) -> i64 {
        if power_mw <= 0 || seconds <= 0 {
            return 0;
        }
        let requested = mwh(power_mw, seconds);
        let free = self.capacity_mwh - self.stored_mwh;
        let accepted = if requested < free { requested } else { free };
        self.stored_mwh += accepted;
        accepted
    }

    /// Spend energy only if the protected reserve survives.
    pub fn try_discharge(&mut self, power_mw: i64, seconds: i64) -> Result<(), PowerError> {
        if power_mw < 0 || seconds < 0 {
            return Err(PowerError::InvalidArgument);
        }
        let need = mwh(power_mw, seconds);
        if self.stored_mwh - need < self.reserve_mwh() {
            return Err(PowerError::InsufficientEnergy);
        }
        self.stored_mwh -= need;
        Ok(())
    }

    /// Can a burst of `power_mw` for `seconds` be afforded right now?
    pub fn can_afford(&self, power_mw: i64, seconds: i64) -> bool {
        if power_mw < 0 || seconds < 0 {
            return false;
        }
        self.stored_mwh - mwh(power_mw, seconds) >= self.reserve_mwh()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Numbers from the design document: 13 W for 5 minutes, 1.5 W average
    /// harvest over a 90 minute orbit.
    #[test]
    fn burst_fits_in_one_orbit_of_harvest() {
        let orbit_harvest = mwh(1500, 90 * 60);
        assert_eq!(orbit_harvest, 2250); // 2.25 Wh

        let burst = mwh(13000, 300);
        assert_eq!(burst, 1083); // ~1.08 Wh

        let mut b = Battery::new(orbit_harvest);
        b.charge(1500, 90 * 60);
        assert_eq!(b.stored_mwh(), 2250);
        assert!(b.can_afford(13000, 300));
        b.try_discharge(13000, 300).expect("burst should fit");
        assert_eq!(b.stored_mwh(), 1167);
    }

    #[test]
    fn short_burst_is_cheap() {
        // 80 Mbit at 4.3 Mbps ~= 18.6 s.
        assert_eq!(mwh(13000, 19), 68); // 0.068 Wh
    }

    #[test]
    fn reserve_is_protected() {
        let mut b = Battery::from_soc(2000, 300); // 0.6 Wh stored, 0.4 Wh reserve
        assert_eq!(b.reserve_mwh(), 400);
        assert_eq!(
            b.try_discharge(3000, 600),
            Err(PowerError::InsufficientEnergy)
        );
        assert_eq!(b.try_discharge(100, 60), Ok(()));
    }

    #[test]
    fn charge_saturates_at_capacity() {
        let mut b = Battery::new(1000);
        assert_eq!(b.charge(10_000, 3600), 1000);
        assert_eq!(b.charge(10_000, 3600), 0);
        assert_eq!(b.soc_permille(), 1000);
    }

    #[test]
    fn negative_inputs_are_rejected() {
        let mut b = Battery::from_soc(1000, 1000);
        assert_eq!(b.try_discharge(-1, 10), Err(PowerError::InvalidArgument));
        assert!(!b.can_afford(-1, 10));
    }
}

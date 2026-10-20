// =====================================================================
//  1U CubeSat packaging model
//
//  Parametric mechanical envelope for the 1U architecture described in
//  README.md: 100 mm body, 8.5 mm rails, body-mounted solar cells,
//  internal EPS/OBC/radio/payload stack, S-band patch and UHF whip.
//
//  This is a packaging/visual model, not a structural or thermal analysis.
//  Wall thicknesses and clearances are indicative.
//
//  Render:
//    openscad -o cubesat.stl cubesat.scad
//    openscad -D 'explode=0.6' -o exploded.stl cubesat.scad
// =====================================================================

/* [Envelope] */
u = 1;                  // 1, 2 or 3 units
explode = 0.0;          // 0 = assembled, 1 = fully exploded
show_stack = true;
show_antenna = true;
show_panels = true;
deploy = 1;             // 1 = whip deployed, 0 = stowed
$fn = 40;

body = 100;
rail_w = 8.5;
rail_len = 113.5 + (u - 1) * 100;
body_h = 100 * u;
z0 = (rail_len - body_h) / 2;
plate_t = 2;
panel_t = 2;
inner = body - 2 * rail_w;

// ---------------------------------------------------------------------
//  Structure
// ---------------------------------------------------------------------
module rails() {
    color("#aeb4bb")
    for (sx = [-1, 1], sy = [-1, 1]) {
        tx = sx > 0 ? body / 2 - rail_w : -body / 2;
        ty = sy > 0 ? body / 2 - rail_w : -body / 2;
        translate([tx, ty, 0]) cube([rail_w, rail_w, rail_len]);
    }
}

module plates() {
    color("#33373d")
    for (z = [z0, z0 + body_h - plate_t]) {
        translate([-inner / 2, -inner / 2, z + (z > z0 ? 45 * explode : 0)])
            cube([inner, inner, plate_t]);
    }
}

module side_panels() {
    color("#101a3a") {
        translate([body / 2 - panel_t + 35 * explode, -inner / 2, z0])
            cube([panel_t, inner, body_h]);
        translate([-body / 2 - 35 * explode, -inner / 2, z0])
            cube([panel_t, inner, body_h]);
        translate([-inner / 2, body / 2 - panel_t + 35 * explode, z0])
            cube([inner, panel_t, body_h]);
        translate([-inner / 2, -body / 2 - 35 * explode, z0])
            cube([inner, panel_t, body_h]);
    }
}

// ---------------------------------------------------------------------
//  Body-mounted solar cells (3 x 4 per 100 mm face)
// ---------------------------------------------------------------------
module solar_cells() {
    cw = 24;
    ch = 21;
    gap = 2;
    rows = 4 * u;
    total_w = 3 * cw + 2 * gap;
    color("#1b2f7a")
    for (ix = [0:2], iy = [0:rows - 1]) {
        y = -total_w / 2 + cw / 2 + ix * (cw + gap);
        z = z0 + 5 + ch / 2 + iy * (ch + gap);
        // +/-X faces
        translate([body / 2 + 35 * explode, y - cw / 2, z - ch / 2])
            cube([0.8, cw, ch]);
        translate([-body / 2 - 0.8 - 35 * explode, y - cw / 2, z - ch / 2])
            cube([0.8, cw, ch]);
        // +/-Y faces
        translate([y - cw / 2, body / 2 + 35 * explode, z - ch / 2])
            cube([cw, 0.8, ch]);
        translate([y - cw / 2, -body / 2 - 0.8 - 35 * explode, z - ch / 2])
            cube([cw, 0.8, ch]);
    }
}

// ---------------------------------------------------------------------
//  Internal stack: 4 PCBs on standoffs, one per subsystem
// ---------------------------------------------------------------------
module board(z) {
    color("#1d6b3a") translate([-41, -48, z]) cube([82, 96, 1.6]);
}

module stack() {
    color("#c7c9cc")
    for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * 36, sy * 43, z0 + 8]) cylinder(h = body_h - 16, r = 1.6);

    for (i = [0:3])
        board(z0 + 12 + i * 20 + i * 10 * explode);

    // EPS, OBC, radio, payload
    color("#2a2d31") translate([-25, -20, z0 + 13.6]) cube([50, 40, 12]);
    color("#2a2d31") translate([-20, -15, z0 + 33.6]) cube([40, 30, 8]);
    color("#2a2d31") translate([-17, -15, z0 + 53.6]) cube([34, 30, 8]);
    color("#3b3f45") translate([-30, -25, z0 + 73.6]) cube([60, 50, 18]);
}

// ---------------------------------------------------------------------
//  Antennas and external interfaces
// ---------------------------------------------------------------------
module antennas() {
    // S-band patch on the nadir face
    color("#d4af37") translate([-25, -25, z0 - 3]) cube([50, 50, 3]);

    // UHF deployer + whip
    color("#b87333") translate([28, -8, rail_len - 9]) cube([12, 16, 9]);
    color("#b87333")
        translate([34, 0, rail_len - 5])
            rotate([0, deploy ? -35 : 90, 0])
                cylinder(h = deploy ? 170 : 55, r = 0.9);
}

module interfaces() {
    // Remove-before-flight switch
    color("#c0392b") translate([body / 2 - 1, 18, z0 + 20]) cube([1.2, 10, 4]);
    // Access / data port on the +X panel
    color("#7f8c8d") translate([body / 2 + 0.1 + 35 * explode, -34, z0 + 8])
        cube([1, 14, 9]);
}

// ---------------------------------------------------------------------
module cubesat() {
    rails();
    plates();
    if (show_panels) {
        side_panels();
        solar_cells();
        interfaces();
    }
    if (show_stack) stack();
    if (show_antenna) antennas();
}

cubesat();

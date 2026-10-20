// =====================================================================
//  1U CubeSat packaging model
//
//  Parametric mechanical envelope for the 1U architecture described in
//  README.md: 100 mm body, 8.5 mm rails, body-mounted solar cells,
//  internal EPS/OBC/radio/payload stack, S-band patch and UHF whip.
//
//  Packaging/visual model only. It is NOT a structural or thermal
//  analysis; those require FEA and a thermal solver, not a CAD envelope.
//
//  Render:
//    openscad -o cubesat.stl cubesat.scad
//    openscad -D 'explode=0.6' -o exploded.stl cubesat.scad
//    openscad -D 'show_panels=false' -o stack.stl cubesat.scad
// =====================================================================

/* [Envelope] */
u = 1;                  // 1, 2 or 3 units
explode = 0.0;          // 0 = assembled, 1 = fully exploded
show_stack = true;
show_antenna = true;
show_panels = true;
show_mli = true;        // gold internal thermal blanket liners
deploy = 1;             // 1 = whip deployed, 0 = stowed
$fn = 48;

body = 100;
rail_w = 8.5;
rail_chamfer = 0.8;
rail_len = 113.5 + (u - 1) * 100;
body_h = 100 * u;
z0 = (rail_len - body_h) / 2;
plate_t = 2;
panel_t = 2;
inner = body - 2 * rail_w;

// ---------------------------------------------------------------------
//  Structure
// ---------------------------------------------------------------------
module rail_profile(w, c) {
    polygon(points = [
        [c, 0], [w - c, 0], [w, c], [w, w - c],
        [w - c, w], [c, w], [0, w - c], [0, c]
    ]);
}

module rails() {
    color("#aeb4bb")
    for (sx = [-1, 1], sy = [-1, 1]) {
        tx = sx > 0 ? body / 2 - rail_w : -body / 2;
        ty = sy > 0 ? body / 2 - rail_w : -body / 2;
        translate([tx, ty, 0]) linear_extrude(height = rail_len)
            rail_profile(rail_w, rail_chamfer);
    }
}

module plates() {
    color("#33373d")
    for (z = [z0, z0 + body_h - plate_t]) {
        lift = z > z0 ? 45 * explode : 0;
        // 1 mm overlap into the rails on each side: watertight union.
        translate([-(inner + 1) / 2, -(inner + 1) / 2, z + lift])
            cube([inner + 1, inner + 1, plate_t]);
    }
}

module side_panels() {
    color("#101a3a") {
        translate([body / 2 - panel_t + 35 * explode, -(inner + 1) / 2, z0])
            cube([panel_t, inner + 1, body_h]);
        translate([-body / 2 - 35 * explode, -(inner + 1) / 2, z0])
            cube([panel_t, inner + 1, body_h]);
        translate([-(inner + 1) / 2, body / 2 - panel_t + 35 * explode, z0])
            cube([inner + 1, panel_t, body_h]);
        translate([-(inner + 1) / 2, -body / 2 - 35 * explode, z0])
            cube([inner + 1, panel_t, body_h]);
    }
}

// Gold MLI liners just inside the panels; visible in cutaway/exploded views.
module mli() {
    color("#c9a227") {
        translate([body / 2 - panel_t - 0.3, -40, z0 + 2])
            cube([0.3, 80, body_h - 4]);
        translate([-body / 2 + panel_t, -40, z0 + 2])
            cube([0.3, 80, body_h - 4]);
        translate([-40, body / 2 - panel_t - 0.3, z0 + 2])
            cube([80, 0.3, body_h - 4]);
        translate([-40, -body / 2 + panel_t, z0 + 2])
            cube([80, 0.3, body_h - 4]);
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
        // 0.4 mm embedded in the panel, 0.4 mm proud: no coincident faces.
        translate([body / 2 - 0.4 + 35 * explode, y - cw / 2, z - ch / 2])
            cube([0.8, cw, ch]);
        translate([-body / 2 - 0.4 - 35 * explode, y - cw / 2, z - ch / 2])
            cube([0.8, cw, ch]);
        translate([y - cw / 2, body / 2 - 0.4 + 35 * explode, z - ch / 2])
            cube([cw, 0.8, ch]);
        translate([y - cw / 2, -body / 2 - 0.4 - 35 * explode, z - ch / 2])
            cube([cw, 0.8, ch]);
    }
}

// ---------------------------------------------------------------------
//  Internal stack: EPS, OBC, radio, payload
// ---------------------------------------------------------------------
module board(z) {
    color("#1d6b3a") translate([-41, -48, z]) cube([82, 96, 1.6]);
}

module stack() {
    // Standoffs (M3) and their screw heads.
    color("#c7c9cc")
    for (sx = [-1, 1], sy = [-1, 1]) {
        translate([sx * 36, sy * 43, z0 + 6])
            cylinder(h = body_h - 12, r = 1.6);
        translate([sx * 36, sy * 43, z0 + 5.2])
            cylinder(h = 1.2, r = 3.0);
        translate([sx * 36, sy * 43, z0 + body_h - 6.4])
            cylinder(h = 1.2, r = 3.0);
    }

    for (i = [0:3])
        board(z0 + 12 + i * 20 + i * 10 * explode);

    // Each box overlaps its board by 0.2 mm.
    color("#d35400") translate([-25, -20, z0 + 13.4]) cube([50, 40, 12]);  // EPS
    color("#2e86c1") translate([-20, -15, z0 + 33.4 + 10 * explode]) cube([40, 30, 8]);   // OBC
    color("#8e44ad") translate([-17, -15, z0 + 53.4 + 20 * explode]) cube([34, 30, 8]);   // radio
    color("#c0392b") translate([-30, -25, z0 + 73.4 + 30 * explode])
        cube([60, 50, 18]);                                                // payload

    // Radio PA heatsink and connectors.
    color("#7f8c8d") translate([-8, -10, z0 + 61.2 + 20 * explode]) cube([22, 20, 4]);
    color("#111111") translate([-19, -6, z0 + 35.4 + 10 * explode]) cube([6, 10, 3]);
    color("#111111") translate([-18, -6, z0 + 55.4 + 20 * explode]) cube([5, 8, 3]);

    // Simplified harness along the -X/-Y corner, outside the boards.
    color("#1b1b1b")
    for (dx = [0, 2.6]) translate([-45 + dx, -46, z0 + 10])
        cylinder(h = body_h - 20, r = 1.1);
    color("#e67e22") translate([-44, -46.8, z0 + 20]) cube([4, 2, 3]);
    color("#e67e22") translate([-44, -46.8, z0 + 60]) cube([4, 2, 3]);
}

// ---------------------------------------------------------------------
//  Antennas, deployer, external interfaces
// ---------------------------------------------------------------------
module antennas() {
    // S-band patch on the nadir face, 0.5 mm into the bottom plate.
    color("#d4af37") translate([-25, -25, z0 - 2.5]) cube([50, 50, 3]);

    // UHF deployer: housing, hinge pin, burn wire.
    color("#b87333") translate([28, -8, rail_len - 9]) cube([12, 16, 9]);
    color("#7f8c8d") translate([26, -10, rail_len - 4])
        rotate([0, 90, 0]) cylinder(h = 18, r = 1.0);
    color("#c0392b") translate([30, -9, rail_len - 6]) cylinder(h = 4, r = 0.45);

    color("#b87333")
    translate([34, 0, rail_len - 3])
        rotate([0, deploy ? -35 : 90, 0])
            cylinder(h = deploy ? 170 : 55, r = 0.9);
}

module interfaces() {
    // Remove-before-flight switch (+X panel).
    color("#c0392b") translate([body / 2 - 0.8, 18, z0 + 20]) cube([1.2, 10, 4]);
    // Access / data port (+X panel), embedded 0.3 mm.
    color("#7f8c8d") translate([body / 2 - 0.3 + 35 * explode, -34, z0 + 8])
        cube([1, 14, 9]);
}

// ---------------------------------------------------------------------
module cubesat() {
    union() {
        rails();
        plates();
        if (show_panels) {
            side_panels();
            solar_cells();
            interfaces();
        }
        if (show_mli) mli();
        if (show_stack) stack();
        if (show_antenna) antennas();
    }
}

cubesat();

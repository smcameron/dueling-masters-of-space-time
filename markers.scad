/* Can be used as markers to mark known clear paths */

$fn=64;


module marker()
{
	difference() {
		cylinder(h = 3, r1 = 5, r2 = 5);
		translate(v = [0, 0, 2])
			cylinder(h = 20, r1 = 3.5, r2 = 3.5);
	}
}

translate(v = [-11 * 5, -11 * 5, 0]) {
	for (y = [0 : 9]) {
		for (x = [0 : 9]) {
			translate(v = [x * 11, y * 11, 0])
				marker();
		}
	}
}

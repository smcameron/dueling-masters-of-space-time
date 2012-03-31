
diameter = 25.4;
diameter_slop = 3;
height = 4;
height_slop = 0.35;
ncolumns = 4;
nrows = 12;
extra_height = 5;
extra_depth = 6;
extra_width = 6;

module box_hole(i)
{
	hole_width = diameter + diameter_slop / 2;
	translate(v = [ extra_width / 2 + 
			i * (diameter + diameter_slop) + diameter_slop / 4 +
			hole_width / 2,
			(nrows * (height + height_slop) + extra_depth) / 2,
			0]) { 
		translate(v = [0, 0, diameter * 1.25])
		cube(size = [hole_width,
				1.07 * nrows * (height + height_slop),
				diameter / 2 + extra_height], center = true);
		translate(v = [0, 0, 1.8 * diameter / 2])
		rotate(a = 90, v = [1, 0, 0])
			cylinder(h = 1.07 * nrows * (height + height_slop),
				r1 = (diameter + diameter_slop / 2) / 2,
				r2 = (diameter + diameter_slop / 2) / 2,
				// r2 = (diameter + diameter_slop / 2) / 2,
				center = true);
	}
}
module box_bottom_solid()
{
	translate(v = [extra_width, extra_depth, 0])
	difference() {
		union() {
			cube(size = [ ncolumns * (diameter + diameter_slop) + extra_width,
					nrows * (height + height_slop) + extra_depth,
					diameter + extra_height],
					center = false);
			translate(v = [-extra_width/4, -extra_depth/4, 0])
			cube(size = [ ncolumns * (diameter + diameter_slop) + extra_width * 1.5,
					nrows * (height + height_slop) + extra_depth * 1.5,
					diameter],
					center = false);
		}
		translate(v = [0, 0, -0.25 * diameter]) {
			box_hole(0);
			box_hole(1);
			box_hole(2);
			box_hole(3);
		}
	}
}

module box_lid()
{
	difference() {
		cube(size = [ ncolumns * (diameter + diameter_slop) + extra_width * 1.5,
				nrows * (height + height_slop) + extra_depth * 1.5,
				extra_height * 1.8],
				center = false);
		translate(v = [extra_width / 5, extra_depth / 5, 3]) {
			cube(size = [ ncolumns * (diameter + diameter_slop) + extra_width * 1.5 - 2 * extra_width / 5,
				nrows * (height + height_slop) + extra_depth * 1.5 - 2 * extra_depth / 5,
				extra_height * 1.5],
				center = false);
		}
	}
}



box_bottom_solid();

translate (v = [0, nrows * (height + height_slop) + extra_depth * 3, 0])
	box_lid();



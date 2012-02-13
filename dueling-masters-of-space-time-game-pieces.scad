/*
 * This file defines 3-d printable 
 * Game pieces for the game Dueling Masters of Space Time
 *
 * (c) Copyright 2012, Stephen M. Cameron
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

diameter = 25.4;
height = 4;

module basic_piece()
{
	difference() {
		cylinder(h = height, 
			r1 = diameter / 2, r2 = diameter / 2,
			center = true);
		translate(v = [0, 0, height * 0.65]) {
			cylinder(h = height, 
				r1 = 0.85 * (diameter / 2),
				r2 = 0.85 * (diameter / 2),
				center = true);
		}
	}
}

module bpxy(x, y)
{
	translate(v = [x, y, 0])
		basic_piece();
}

module planet_ring()
{
	scale(v = [1, 0.20, 1]) {
		difference() {
			cylinder(h = height, 
				r1 = diameter / 2, r2 = diameter / 2,
				center = true);
			cylinder(h = height * 2, 
				r1 = 0.85 * (diameter / 2),
				r2 = 0.85 * (diameter / 2),
				center = true);
		}
	}
}

module planet_body()
{
	cylinder(h = height,
		r1 = diameter / 4,
		r2 = diameter / 4,
		center = true);
}

module planet()
{
	scale(v = [0.8, 0.8, 1.0]) {
		union() {
			planet_body();
			planet_ring();
		}
	}
}

module home_planet(x, y)
{
	translate(v = [x, y, 0])
	union() {
		planet();
		basic_piece();
	}
}

module X()
{
	difference() {
		union() {
			rotate(a = 30, v = [0, 0, 1])
				cube(size = [diameter * 0.1, diameter, height],
					center = true);		
			rotate(a = -30, v = [0, 0, 1])
				cube(size = [diameter * 0.1, diameter, height],
					center = true);		
		}
		translate(v = [0, 0.8 * (diameter / 2), 0])
			cube(size = [diameter, diameter * 0.2, height * 2], center = true);
		translate(v = [0, -0.8 * (diameter / 2), 0])
			cube(size = [diameter, diameter * 0.2, height * 2], center = true);
	}
}

module Y()
{
	difference() {
		union() {
			translate(v = [0, -diameter / 4, 0])
				cube(size = [diameter * 0.15, 0.6 * diameter, height],
					center = true);
			rotate(a = -(90 + 60), v = [0, 0, 1])
				translate(v = [0, -diameter / 4, 0])
					cube(size = [diameter * 0.15, 0.6 * diameter, height],
						center = true);
			rotate(a = (90 + 60), v = [0, 0, 1])
				translate(v = [0, -diameter / 4, 0])
					cube(size = [diameter * 0.15, 0.6 * diameter, height],
						center = true);
		}
		translate(v = [0, 0.90 * (diameter / 2), 0])
			cube(size = [diameter, diameter * 0.3, height * 2], center = true);
		translate(v = [0, -0.90 * (diameter / 2), 0])
			cube(size = [diameter, diameter * 0.3, height * 2], center = true);
	}
}

module Z()
{
	union() {
	translate(v = [0, -0.4 * (diameter / 2), 0])
		cube(size = [diameter * 0.4, diameter * 0.1, height],
			center = true);
	translate(v = [0, 0.4 * (diameter / 2), 0])
		cube(size = [diameter * 0.4, diameter * 0.1, height],
			center = true);
	rotate(a = -42, v = [0, 0, 1])
		cube(size = [diameter * 0.1, diameter * 0.50, height],
				center = true);
	}
}

module V()
{
	union() {
		translate(v = [-diameter * 0.09, 0, 0])
			rotate(a = 15, v = [0, 0, 1])
				cube(size = [diameter * 0.1,
					diameter * 0.75,
					height],
					center = true);
		translate(v = [diameter * 0.09, 0, 0])
			rotate(a = -15, v = [0, 0, 1])
				cube(size = [diameter * 0.1,
					diameter * 0.75,
					height],
					center = true);
	}
}

module W()
{
	difference() {
		union() {
			translate(v = [-diameter * 0.10, 0, 0 ])
				V();	
			translate(v = [ diameter * 0.10, 0, 0 ])
				V();	
		}
		translate(v = [0, diameter * 0.25, 0])
			cube(size = [diameter * 0.27,
					diameter * 0.33,
					height * 2.0],
					center = true);
		translate(v = [0, diameter * 0.35, 0])
			cube(size = [diameter, 
					diameter * 0.2,
					height * 2],
					center = true);
	}
}

module B()
{
	scale(v = [0.80, 0.80, 1.0]) {
	translate(v = [diameter * 0.1, 0, 0])
	union() {
	translate(v = [-diameter * 0.35, 0, 0])
		cube(size = [diameter * 0.1,
			diameter * 0.8,
			height],
			center = true);
	translate(v = [-diameter * 0.2, -diameter * 0.35, 0])
		cube(size = [diameter * 0.3,
			diameter * 0.1,
			height],
			center = true);
	translate(v = [-diameter * 0.2, diameter * 0.35, 0])
		cube(size = [diameter * 0.3,
			diameter * 0.1,
			height],
			center = true);
	translate(v = [-diameter * 0.2, diameter * 0.05, 0])
		cube(size = [diameter * 0.3,
			diameter * 0.1,
			height],
			center = true);
	translate(v = [-diameter * 0.03, -diameter * 0.15, 0])
		difference() {
			cylinder(h = height, r1 = diameter * 0.25,
					r2 = diameter * 0.25,
					center = true);
			cylinder(h = height * 2,
				r1 = diameter * 0.15,
				r2 = diameter * 0.15,
				center = true);
			translate(v = [ - diameter * 0.1, 0, 0])
				cube(size = [diameter * 0.3,
					diameter * 0.29,
					height * 2],
					center = true);
		}
	translate(v = [-diameter * 0.03, diameter * 0.20, 0])
		difference() {
			cylinder(h = height,
				r1 = diameter * 0.2,
				r2 = diameter * 0.2,
				center = true);
			cylinder(h = height * 2,
				r1 = diameter * 0.1,
				r2 = diameter * 0.1,
				center = true);
			translate(v = [ - diameter * 0.1, 0, 0])
				cube(size = [diameter * 0.2,
					diameter * 0.195,
					height * 2],
					center = true);
		}
	}
	}
}

module one()
{
	difference() {
		union() {
			cube(size = [diameter * 0.1,
				diameter * 0.5,
				height], center = true);
			translate(v = [-diameter * 0.05,
					diameter * 0.2,
					0]) {
				cube(size = [diameter * 0.2,
						diameter * 0.2,
						height], center = true);
			}
		}
		translate(v = [-diameter * 0.05,
				diameter * 0.2,
				0]) {
			translate(v = [ -diameter * 0.085,
					diameter * 0.085, 0]) 
				cylinder(h = height * 2,
					r1 = diameter * 0.1,
					r2 = diameter * 0.1, 
					center = true);
		}
	}
}

module two()
{
	difference() {
		cylinder(h = height, 
			r1 = diameter * 0.25,
			r2 = diameter * 0.25,
			center = true);
		cylinder(h = height * 2, 
			r1 = diameter * 0.15,
			r2 = diameter * 0.15,
			center = true);
		translate(v = [0, diameter * -0.35, 0])
			cube(size = [diameter * 0.7,
					diameter * 0.7, height * 2], 
					center = true);
	}
	translate(v = [0, -diameter * 0.17, 0])
		rotate(a = -45, v = [0, 0, 1])
			cube(size = [diameter * 0.1,
					diameter * 0.6, height],
				center = true);
	translate(v = [0, -diameter * 0.38, 0])
		cube(size = [diameter * 0.48, diameter * 0.1, height],
			center = true);
}

module three()
{
	difference() {
		union() {
			translate(v = [0, diameter * 0.25, 0])
				difference() {
					cylinder(h = height,
						r1 = diameter * 0.25,
						r2 = diameter * 0.25,
						center = true);
					cylinder(h = height * 2,
						r1 = diameter * 0.15,
						r2 = diameter * 0.15,
						center = true);
				}
			translate(v = [0, diameter * -0.20, 0])
				difference() {
					cylinder(h = height,
						r1 = diameter * 0.3,
						r2 = diameter * 0.3,
						center = true);
					cylinder(h = height * 2,
						r1 = diameter * 0.20,
						r2 = diameter * 0.20,
						center = true);
				}
		}
		translate(v = [-diameter * 0.25, 0, 0])
			cube(size = [diameter * 0.5,
					diameter * 0.6,
					height * 2],
					center = true);
	}
}

module four()
{
	union() {
		translate(v = [diameter * 0.1, 0, 0 ])
			cube(size = [diameter * 0.1,
				diameter * 0.8,
				height],
				center = true);	
		cube(size = [diameter * 0.5,
				diameter * 0.1,
				height],
				center = true);
		translate(v = [-diameter * 0.17, diameter * 0.2, 0])
			rotate(a = -10, v = [0, 0, 1])
				cube(size = [diameter * 0.1,
						diameter * 0.35,
						height],
						center = true);
	}
}

module x_piece()
{
	union () {
		translate(v = [-diameter * 0.05, 0, 0])
			X();
		basic_piece();
	}
}

module x1_piece(x, y) 
{
	translate(v = [x, y, 0])
		union () {
			x_piece();
			translate(v = [diameter * 0.25, 
					-diameter * 0.15, 0])
				scale(v = [0.6, 0.6, 1])
					one();
		}
}

module x2_piece(x, y)
{
	translate(v = [x, y, 0])
	union () {
		x_piece();
		translate(v = [diameter * 0.25, 
				0, 0])
			scale(v = [0.45, 0.45, 1])
				two();
	}
}

module z1_piece(x, y)
{
	translate(v = [x, y, 0])
	union () {
		z_piece();
		translate(v = [diameter * 0.25, 
				0, 0])
			scale(v = [0.5, 0.5, 1])
				one();
	}
}

module z2_piece(x, y)
{
	translate(v = [x, y, 0])
	union () {
		z_piece();
		translate(v = [diameter * 0.25, 
				diameter * 0.05, 0])
			scale(v = [0.4, 0.4, 1])
				two();
	}
}

module z3_piece(x, y)
{
	translate(v = [x, y, 0])
	union () {
		z_piece();
		translate(v = [diameter * 0.25, 
				0, 0])
			scale(v = [0.4, 0.4, 1])
				three();
	}
}

module z4_piece(x, y)
{
	translate(v = [x, y, 0])
	union () {
		z_piece();
		translate(v = [diameter * 0.25, 
				0, 0])
			scale(v = [0.4, 0.4, 1])
				four();
	}
}

module y_piece()
{
	union () {
		translate(v = [-diameter * 0.05, 0, 0])
			Y();
		basic_piece();
	}
}

module y1_piece(x, y)
{
	translate(v = [x, y, 0])
	union () {
		y_piece();
		translate(v = [diameter * 0.25, 
				0, 0])
			scale(v = [0.6, 0.6, 1])
				one();
	}
}

module y2_piece(x, y)
{
	translate(v = [x, y, 0])
	union () {
		y_piece();
		translate(v = [diameter * 0.25, 
				0, 0])
			scale(v = [0.45, 0.45, 1])
				two();
	}
}

module z_piece()
{
	union() {
		translate(v = [-diameter * 0.08, 0, 0])
			Z();
		basic_piece();
	}
}

module w_piece(x, y)
{
	translate(v = [x, y, 0])
	union() {
		W();
		basic_piece();
	}
}

module b_piece(x, y)
{
	translate(v = [x, y, 0])
	union() {
		B();
		basic_piece();
	}
}

initx = 0;
inity = 0;

x = initx;
y = inity;

space = (1.2 * diameter);

translate(v = [space / 2, space / 2, height / 2]) {
	home_planet(x + space * 0, y + space * 0);
	x1_piece(x + space * 1, y + space * 0);
	x2_piece(x + space * 2, y + space * 0);
	y1_piece(x + space * 3, y + space * 0);
	y2_piece(x + space * 0, y + space * 1);
	z1_piece(x + space * 1, y + space * 1);
	z2_piece(x + space * 2, y + space * 1);
	z3_piece(x + space * 3, y + space * 1);
	z4_piece(x + space * 0, y + space * 2);
	w_piece(x + space * 1, y + space * 2);
	b_piece(x + space * 2, y + space * 2);
}

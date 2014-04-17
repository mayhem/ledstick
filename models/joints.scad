

// U-shape
width       = 12.0;
height      = 16.0;
thickness   = 1.9;

// object size
o_width = 20;
o_height = 20;
o_thickness = 3;

//test_base();
//u_shape(o_thickness);
end_cap(20);

module test_base()
{
    difference()
    {
		cube([o_width, o_height, o_thickness], center=true);
        u_shape(o_thickness);
    }
}

module end_cap(r)
{
    difference()
	{
    		difference()
		{
    			sphere(r, $fn=10);
        		translate([0, 0, r]) 
        			u_shape(r);
		}
		//translate([0,0,-r])
			cube([r * 2, r * 2, r], center=true);
    }
}

module u_shape(t)
{
    difference()
    {
         cube([width + (thickness * 2), height, t], center=true);
         translate([0, thickness, 0])
             cube([width, height, t], center=true);
    }
} 
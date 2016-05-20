﻿// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Singleplayer {

	public class FoilagePhysics {		
		Game game;
		World map;
		Random rnd = new Random();
		
		public FoilagePhysics( Game game ) {
			this.game = game;
			map = game.World;
		}
		
		// Algorithm source: Looking at the trees generated by the default classic server.
		// Hence, the random thresholds may be slightly off.
		public void GrowTree( int x, int y, int z ) {
			int trunkH = rnd.Next( 1, 4 );
			game.UpdateBlock( x, y, z, 0 );
			
			// Can the new tree grow?
			if( !CheckBounds( x, x, y, y + trunkH - 1, z, z ) ||
			   !CheckBounds( x - 2, x + 2, y + trunkH, y + trunkH + 1, z - 2, z + 2 ) ||
			   !CheckBounds( x - 1, x + 1, y + trunkH + 2, y + trunkH + 3, z - 1, z + 1 ) ) {
				game.UpdateBlock( x, y, z, 0 );
				return;
			}
			
			// Leaves bottom layer
			y += trunkH;
			for( int zz = -2; zz <= 2; zz++ ) {
				for( int xx = -2; xx <= 2; xx++ ) {
					if( Math.Abs( xx ) == 2 && Math.Abs( zz ) == 2 ) {
						if( rnd.Next( 0, 5 ) < 4 ) game.UpdateBlock( x + xx, y, z + zz, (byte)Block.Leaves );
						if( rnd.Next( 0, 5 ) < 2 ) game.UpdateBlock( x + xx, y + 1, z + zz, (byte)Block.Leaves );
					} else {
						game.UpdateBlock( x + xx, y, z + zz, (byte)Block.Leaves );
						game.UpdateBlock( x + xx, y + 1, z + zz, (byte)Block.Leaves );
					}
				}
			}
			
			// Leaves top layer
			y += 2;
			for( int zz = -1; zz <= 1; zz++ ) {
				for( int xx = -1; xx <= 1; xx++ ) {
					if( xx == 0 || zz == 0 ) {
						game.UpdateBlock( x + xx, y, z + zz, (byte)Block.Leaves );
						game.UpdateBlock( x + xx, y + 1, z + zz, (byte)Block.Leaves );
					} else if( rnd.Next( 0, 5 ) == 0 ) {
						game.UpdateBlock( x + xx, y, z + zz, (byte)Block.Leaves );
					}
				}
			}
			
			// Base trunk
			y -= 2 + trunkH;
			for( int yy = 0; yy < trunkH + 3; yy++ )
				game.UpdateBlock( x, y + yy, z, (byte)Block.Log );
		}
		
		bool CheckBounds( int x1, int x2, int y1, int y2, int z1, int z2 ) {
			for( int y = y1; y <= y2; y++ )
				for( int z = z1; z <= z2; z++ )
					for( int x = x1; x <= x2; x++ )
			{
				if( !map.IsValidPos( x, y, z ) ) return false;
				
				byte block = map.GetBlock( x, y, z );
				if( !(block == 0 || block == (byte)Block.Leaves ) ) return false;
			}
			return true;
		}
	}
}
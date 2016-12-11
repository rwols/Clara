#pragma once

enum X { x };

void foo()
{
	X z = x;
	switch (z)
	{
		case x: break;
		default: break;
	}
}

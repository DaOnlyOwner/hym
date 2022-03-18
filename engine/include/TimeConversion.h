#pragma once
#include "Definitions.h"

namespace Hym
{
	class Time
	{
	public:
		Time(u64 nano)
			:nano(nano){}
		double Secs()
		{
			return nano / 1000. / 1000. / 1000.;
		}
		double MilliSecs()
		{
			return nano / 1000. / 1000.;
		}
		double MicroSecs()
		{
			return nano / 1000.;
		}
		u64 NanoSecs()
		{
			return nano;
		}
	private:
		u64 nano;
	};
}
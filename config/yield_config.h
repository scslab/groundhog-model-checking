#pragma once


namespace scs
{

struct YieldConfig {
	bool UINT128_YIELD = false;
	bool HS_YIELD = false;
	bool RO_YIELD = false;
	bool RBO_YIELD = false;
	bool RBO_U128_YIELD = false;
};

extern YieldConfig yield_config;

}


#include "stdafx.h"
#include "TwitchMessageParams.h"

namespace Twitch::irc::parameters {
	bool operator==(const Color& lhs, const Color& rhs) {
		if (lhs.initialized != rhs.initialized) { return false; }

		return lhs.r == rhs.r
			&& lhs.g == rhs.g
			&& lhs.b == rhs.b;
	}
	bool operator!=(const Color& lhs, const Color& rhs) {
		return !(lhs == rhs);
	}
}
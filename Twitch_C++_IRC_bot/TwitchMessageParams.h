#ifndef TWITCHMESSAGEPARAMS_H
#define TWITCHMESSAGEPARAMS_H

#include <chrono>
#include <string>
#include <string_view>
#include <optional>
#include <map>

namespace Twitch::irc::parameters {
	using timestamp_t = std::chrono::seconds;

	// TODO: separate classes for users, add user list for channel
	struct NoColor {};
	struct Color {
		bool initialized{ false };
		const int r{ 0 }, g{ 0 }, b{ 0 };

		static inline auto from_string(
			const std::string& r, const std::string& g, const std::string& b
		) {
			auto hex_to_int = [](const std::string& raw_hex) {
				return std::stoi(raw_hex, 0, 16);
			};

			return Color{ hex_to_int(r), hex_to_int(g), hex_to_int(b) };
		}

		constexpr Color(NoColor) {};
		constexpr Color(int t_r, int t_g, int t_b) noexcept
			: initialized(true), r(t_r), g(t_g), b(t_b)
		{}

		friend bool operator==(const Color& lhs, const Color& rhs);
		friend bool operator!=(const Color& lhs, const Color& rhs);
	};

	// TODO: find missig badges and add
	using BadgeLevel = int;
	struct Badge {
		enum Type {
			unhandled_badge = -1,
			bits, turbo, subscriber, moderator,
			broadcaster, global_mod, admin, staff
		};

		const Type type{ unhandled_badge };

		constexpr Badge(Type t) noexcept : type(t) {}

		friend constexpr bool operator==(Badge lhs, Badge rhs) noexcept {
			return lhs.type == rhs.type;
		}
		friend constexpr bool operator!=(Badge lhs, Badge rhs) noexcept {
			return !(lhs.type == rhs.type);
		}
		friend constexpr bool operator>(Badge lhs, Badge rhs) noexcept {
			return lhs.type > rhs.type;
		}
		friend constexpr bool operator<(Badge lhs, Badge rhs) noexcept {
			return lhs.type < rhs.type;
		}

		static constexpr auto from_string(std::string_view raw) noexcept;
		static constexpr auto to_string(Badge badge) noexcept;
	};
	
	constexpr auto Badge::from_string(std::string_view raw) noexcept {
		using namespace std::string_view_literals;
		if (raw == "subscriber"sv) { return Badge::subscriber; }
		if (raw == "bits"sv) { return Badge::bits; }
		if (raw == "turbo"sv) { return Badge::turbo; }
		if (raw == "moderator"sv) { return Badge::moderator; }
		if (raw == "broadcaster"sv) { return Badge::broadcaster; }
		if (raw == "global_mod"sv) { return Badge::global_mod; }
		if (raw == "admin"sv) { return Badge::admin; }
		if (raw == "staff"sv) { return Badge::staff; }

		return Badge::unhandled_badge;
	}
	constexpr auto Badge::to_string(Badge badge) noexcept {
		if (badge == Badge::subscriber) { return "subscriber"; }
		if (badge == Badge::bits) { return "bits"; }
		if (badge == Badge::turbo) { return "turbo"; }
		if (badge == Badge::moderator) { return "moderator"; }
		if (badge == Badge::broadcaster) { return "broadcaster"; }
		if (badge == Badge::global_mod) { return "global_mod"; }
		if (badge == Badge::admin) { return "admin"; }
		if (badge == Badge::staff) { return "staff"; }

		return "unhandled_badge";
	}

	enum class UserPrivilegesLevel : int {
		normal = 0, regular, subscriber, moderator, broadcaster
	};

	struct UserType {
		enum Type {
			unhandled_type = -1,
			empty, // default
			mod, global_mod, admin, staff
		};

		const Type type{ empty };

		constexpr UserType(Type t) noexcept : type(t) {}

		friend constexpr bool operator==(UserType lhs, UserType rhs) noexcept {
			return lhs.type == rhs.type;
		}
		friend constexpr bool operator!=(UserType lhs, UserType rhs) noexcept {
			return !(lhs.type == rhs.type);
		}

		static constexpr auto from_string(std::string_view raw) noexcept;
		static constexpr auto to_string(UserType utype) noexcept;
	};

	constexpr auto UserType::from_string(std::string_view raw) noexcept {
		using namespace std::string_view_literals;
		// most common case
		if (raw.empty()) { return UserType::empty; }
		if (raw == "mod"sv) { return UserType::mod; }
		if (raw == "global_mod"sv) { return UserType::global_mod; }
		if (raw == "admin"sv) { return UserType::admin; }
		if (raw == "staff"sv) { return UserType::staff; }

		return UserType::unhandled_type;
	}
	constexpr auto UserType::to_string(UserType utype) noexcept {
		if (utype == UserType::empty) { return ""; }
		if (utype == UserType::mod) { return "mod"; }
		if (utype == UserType::global_mod) { return "global_mod"; }
		if (utype == UserType::admin) { return "admin"; }
		if (utype == UserType::staff) { return "staff"; }

		return "unhandled_type";
	}


	// Badges, UserType, Color
	template<class Logger>
	Logger& operator<<(Logger& logger, Color color) {
		if (!color.initialized) { return logger; }
		return logger << '#'
			<< std::hex << std::uppercase
			<< (color.r <= 0xF ? "0" : "") << color.r
			<< (color.g <= 0xF ? "0" : "") << color.g
			<< (color.b <= 0xF ? "0" : "") << color.b
			<< std::dec << std::nouppercase;
	}
	template<class Logger>
	Logger& operator<<(Logger& logger, Badge badge) {
		return logger << Badge::to_string(badge);
	}
	template<class Logger>
	Logger& operator<<(Logger& logger, const std::map<Badge, BadgeLevel>& badges) {
		const auto first = std::begin(badges);
		const auto end = std::end(badges);

		if (first == end) { return logger; }

		logger << first->first << '/' << first->second;
		for (auto it = std::next(first); it != end; ++it) {
			logger << ' ' << it->first << '/' << it->second;
		}

		return logger;
	}
	template<class Logger>
	Logger& operator<<(Logger& logger, UserType utype) {
		return logger << UserType::to_string(utype);
	}
	//
}

#endif // !TWITCHMESSAGEPARAMS_H
#ifndef GEOHASH_H_
#define GEOHASH_H_

#include <cstdint>
#include <array>
#include <string_view>
#include <string>
#include <cmath>

namespace GeoHash {
	constexpr static size_t MAX_SIZE = 12;

	using buffer_t    = std::array<char, 32>; // 12

	// ------------------------------

	struct Point{
		double lat;
		double lon;
	};

	struct GeoSphere;

	struct Rectangle{
		Point sw;
		Point ne;

		constexpr auto n_lat() const{
			return ne.lat;
		};

		constexpr auto s_lat() const{
			return sw.lat;
		};

		constexpr auto e_lon() const{
			return ne.lon;
		};

		constexpr auto w_lon() const{
			return sw.lon;
		};

		// ------------------------------

		constexpr Point ne_p() const{
			return { n_lat(), e_lon() };
		};

		constexpr Point nw_p() const{
			return { n_lat(), w_lon() };
		};

		constexpr Point se_p() const{
			return { s_lat(), e_lon() };
		};

		constexpr Point sw_p() const{
			return { s_lat(), w_lon() };
		};

		// ------------------------------

		constexpr Point center() const{
			return {
				(sw.lat + ne.lat) / 2,
				(sw.lon + ne.lon) / 2
			};
		}

		// ------------------------------

		double height(GeoSphere const &sphere) const;
		double width(GeoSphere const &sphere) const;
		double area(GeoSphere const &sphere) const;
	};

	enum class Direction : uint8_t{
		n = 0,
		s = 1,
		e = 2,
		w = 3
	};

	// ------------------------------

	struct GeoSphere{
		std::string_view	sphere;

		std::string_view	units;

		double			radius;

		double cells[MAX_SIZE] {
			CELLS_ABSOLUTE[ 0] * radius, //  1
			CELLS_ABSOLUTE[ 1] * radius, //  2
			CELLS_ABSOLUTE[ 2] * radius, //  3
			CELLS_ABSOLUTE[ 3] * radius, //  4
			CELLS_ABSOLUTE[ 4] * radius, //  5
			CELLS_ABSOLUTE[ 5] * radius, //  6
			CELLS_ABSOLUTE[ 6] * radius, //  7
			CELLS_ABSOLUTE[ 7] * radius, //  8
			CELLS_ABSOLUTE[ 8] * radius, //  9
			CELLS_ABSOLUTE[ 9] * radius, // 10
			CELLS_ABSOLUTE[10] * radius, // 11
			CELLS_ABSOLUTE[11] * radius  // 12
		};

	public:
		constexpr static double RADIUS_EARTH_KM		= 6371;
		constexpr static double RADIUS_EARTH_MI		= 3956;

	private:
		constexpr static double CELL_SHRINK_FACTOR	= 0.6666;

		constexpr static double CELL_SHRINK = CELL_SHRINK_FACTOR / RADIUS_EARTH_KM;

		constexpr static inline double CELLS_ABSOLUTE[MAX_SIZE] {
			4'992.600	* CELL_SHRINK, // 1
			  624.100	* CELL_SHRINK, // 2
			  156.100	* CELL_SHRINK, // 3
			   19.500	* CELL_SHRINK, // 4
			    4.900	* CELL_SHRINK, // 5
			    0.609'4	* CELL_SHRINK, // 6
			    0.152'4	* CELL_SHRINK, // 7
			    0.019'0	* CELL_SHRINK, // 8
			    0.004'8	* CELL_SHRINK, // 9
			    0.000'595	* CELL_SHRINK, // 10
			    0.000'149	* CELL_SHRINK, // 11
			    0.000'019	* CELL_SHRINK  // 12
		};
	};

	constexpr GeoSphere EARTH_KM		{ "Earth",	"km",	GeoSphere::RADIUS_EARTH_KM	};
	constexpr GeoSphere EARTH_METERS	{ "Earth",	"m",	EARTH_KM.radius * 1000		};

	constexpr GeoSphere EARTH_MI		{ "Earth",	"mi",	GeoSphere::RADIUS_EARTH_MI	};
	constexpr GeoSphere EARTH_YD		{ "Earth",	"yd",	EARTH_MI.radius * 1760		};

	constexpr GeoSphere MOON_KM		{ "Moon",	"km",	1737.1				};
	constexpr GeoSphere MOON_METERS		{ "Moon",	"m",	MOON_KM.radius * 1000		};

	constexpr GeoSphere MARS_KM		{ "Mars",	"km",	3389.5				};
	constexpr GeoSphere MARS_METERS		{ "Mars",	"m",	MARS_KM.radius * 1000		};

	// ------------------------------

	struct HashVector{
		constexpr auto begin() const{
			return data;
		}

		constexpr auto end() const{
			return data + size;
		}

	private:
		friend HashVector nearbyCells(double, double, double, GeoSphere const &);
		friend HashVector nearbyCells(std::string_view) noexcept;

		std::string_view	data[9];
		size_t			size;

		buffer_t		buffer[9];
	};

	// ------------------------------

	std::string_view encode(double lat, double lon, size_t precision, buffer_t &buffer) noexcept;

	inline auto encode(Point p, size_t precision, buffer_t &buffer){
		return encode(p.lat, p.lon, precision, buffer);
	}

	inline std::string encode(double lat, double lon, size_t precision){
		buffer_t buffer;

		return std::string{
			encode(lat, lon, precision, buffer)
		};
	}

	Rectangle decode(std::string_view hash);

	std::string_view adjacent(std::string_view geohash, Direction direction, buffer_t &buffer) noexcept;

	[[deprecated]]
	HashVector nearbyCells(std::string_view hash) noexcept;

	// ------------------------------

	double distance_radians(double lat1, double lon1, double lat2, double lon2) noexcept;

	inline double distance(double lat1, double lon1, double lat2, double lon2, GeoSphere const &sphere){
		auto _ = [](double degree){
			// toRadians
			constexpr double one_deg = M_PI / 180;
			return one_deg * degree;
		};

		return distance_radians(_(lat1), _(lon1), _(lat2), _(lon2)) * sphere.radius;
	}

	inline auto distance(Point p1, Point p2, GeoSphere const &sphere){
		return distance(p1.lat, p1.lon, p2.lat, p2.lon, sphere);
	}

	// ------------------------------

	HashVector nearbyCells(double lat, double lon, double radius, GeoSphere const &sphere);

	inline auto nearbyCells(Point p, double radius, GeoSphere const &sphere){
		return nearbyCells(p.lat, p.lon, radius, sphere);
	}

	// ------------------------------

	inline double Rectangle::height(GeoSphere const &sphere) const{
	//	return distance(sw.lat, sw.lon, ne.lat, sw.lon, sphere);
		return distance(ne_p(), se_p(), sphere);
	}

	inline double Rectangle::width(GeoSphere const &sphere) const{
	//	return distance(sw.lat, sw.lon, sw.lat, ne.lon, sphere);
		return distance(ne_p(), nw_p(), sphere);
	}

	inline double Rectangle::area(GeoSphere const &sphere) const{
		return width(sphere) * height(sphere);
	}
}

#endif


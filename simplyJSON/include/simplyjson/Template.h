#include "Common.h"

template<typename T>
struct is_vector : std::false_type {};

template<typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};


template<typename T>
struct is_umap : std::false_type {};

template<typename K, typename V, typename Hash, typename Eq, typename Alloc>
struct is_umap<std::unordered_map<K, V, Hash, Eq, Alloc>>
	: std::bool_constant<std::is_same_v<K, std::string>> {}; 

template<class> inline constexpr bool always_false = false;
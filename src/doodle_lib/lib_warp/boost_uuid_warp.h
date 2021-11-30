#pragma once

#define BOOST_UUID_RANDOM_PROVIDER_FORCE_WINCRYPT
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/uuid/uuid_serialize.hpp>
#include <boost/uuid/uuid_io.hpp>
#undef BOOST_UUID_RANDOM_PROVIDER_FORCE_WINCRYPT
#include <boost/lexical_cast.hpp>

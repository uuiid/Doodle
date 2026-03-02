#pragma once

#include <sqlite_orm/sqlite_orm.h>

#include <array>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace sqlite_orm {

namespace internal {

	struct dynamic_where_entry_base {
		std::string expression;
	};

	/**
	 * C - serializer context class
	 */
	template <class C>
	struct dynamic_where_t : where_string {
		using context_t = C;
		using entry_t = dynamic_where_entry_base;
		using const_iterator = typename std::vector<entry_t>::const_iterator;

		dynamic_where_t(const context_t& context_) : context(context_) {}

		template <class E>
		void push_back(E expression) {
			auto newContext = this->context;
			newContext.omit_table_name = false;
			// NOTE: follow sqlite_orm dynamic_set/dynamic_order_by pattern:
			// store fully serialized expression, with bindables inlined.
			this->entries.push_back({serialize(std::move(expression), newContext)});
		}

		const_iterator begin() const {
			return this->entries.begin();
		}

		const_iterator end() const {
			return this->entries.end();
		}

		bool empty() const {
			return this->entries.empty();
		}

		void clear() {
			this->entries.clear();
		}

	protected:
		std::vector<entry_t> entries;
		context_t context;
	};

	template <class T>
	inline constexpr bool is_dynamic_where_v = polyfill::is_specialization_of<T, dynamic_where_t>::value;

	template <class T>
	struct is_dynamic_where : polyfill::bool_constant<is_dynamic_where_v<T>> {};

	template <class T>
	inline constexpr bool is_where_clause_v = polyfill::disjunction<is_where<T>, is_dynamic_where<T>>::value;

	template <class T>
	struct is_where_clause : polyfill::bool_constant<is_where_clause_v<T>> {};

	template <class C>
	struct statement_serializer<dynamic_where_t<C>, void> {
		using statement_type = dynamic_where_t<C>;

		template <class Ctx>
		std::string operator()(const statement_type& statement, const Ctx&) const {
			if (statement.empty()) {
				return {};
			}

			std::stringstream ss;
			ss << statement.serialize() << " ";

			static constexpr std::array<orm_gsl::czstring, 2> sep = {" AND ", ""};
#ifdef SQLITE_ORM_INITSTMT_RANGE_BASED_FOR_SUPPORTED
			for (bool first = true; const dynamic_where_entry_base& entry : statement) {
				ss << sep[std::exchange(first, false)] << '(' << entry.expression << ')';
			}
#else
			bool first = true;
			for (const dynamic_where_entry_base& entry : statement) {
				ss << sep[std::exchange(first, false)] << '(' << entry.expression << ')';
			}
#endif

			return ss.str();
		}
	};

}  // namespace internal

/**
 * WHERE condition pack.
 * Difference from `where(...)` is that `dynamic_where` can be changed at runtime using `push_back` member.
 */
template <class S>
internal::dynamic_where_t<internal::serializer_context<typename S::db_objects_type>> dynamic_where(const S& storage) {
	return {obtain_db_objects(storage)};
}


}
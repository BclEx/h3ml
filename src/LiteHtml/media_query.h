#pragma once

namespace litehtml
{
	struct media_query_expression
	{
		typedef std::vector<media_query_expression>	vector;

		media_feature feature;
		int val;
		int val2;
		bool check_as_bool;

		media_query_expression()
		{
			check_as_bool = false;
			feature = media_feature_none;
			val = 0;
			val2 = 0;
		}

		bool check(const media_features &features) const;
	};

	class media_query
	{
	public:
		typedef std::shared_ptr<media_query> ptr;
		typedef std::vector<media_query::ptr> vector;
	private:
		media_query_expression::vector _expressions;
		bool _not;
		media_type _media_type;
	public:
		media_query();
		media_query(const media_query &val);

		static media_query::ptr create_fro_string(const tstring &str, const std::shared_ptr<document> &doc);
		bool check(const media_features &features) const;
	};

	class media_query_list
	{
	public:
		typedef std::shared_ptr<media_query_list> ptr;
		typedef std::vector<media_query_list::ptr> vector;
	private:
		media_query::vector	_queries;
		bool _is_used;
	public:
		media_query_list();
		media_query_list(const media_query_list &val);

		static media_query_list::ptr create_fro_string(const tstring &str, const std::shared_ptr<document> &doc);
		bool is_used() const;
		bool apply_media_features(const media_features &features);	// returns true if the _is_used changed
	};

	inline media_query_list::media_query_list(const media_query_list &val)
	{
		_is_used = val._is_used;
		_queries = val._queries;
	}

	inline media_query_list::media_query_list()
	{
		_is_used = false;
	}

	inline bool media_query_list::is_used() const
	{
		return _is_used;
	}
}
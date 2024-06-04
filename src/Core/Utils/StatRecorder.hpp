#pragma once

#include <unordered_map>
#include <string>
#include <cassert>
#include <chrono>

#include <Core/IO/GuiContext.hpp>
#include <Core/DynamicValue.hpp>
#include <Core/Execution/FramePerformanceCounters.hpp>


namespace vkl
{
	class AbstractStatRecord
	{
	public:
		enum class Type
		{
			Float,
			Double,
			Uint32,
			Int32,
			Uint64,
			Int64,
			MAX_ENUM,
		};

		static constexpr size_t GetTypeSize(Type t)
		{
			size_t res = 0;
			switch (t)
			{
			case Type::Float:
				res = sizeof(float);
				break;
			case Type::Double:
				res = sizeof(double);
				break;
			case Type::Uint32:
				res = sizeof(uint32_t);
				break;
			case Type::Int32:
				res = sizeof(int32_t);
				break;
			case Type::Uint64:
				res = sizeof(uint64_t);
				break;
			case Type::Int64:
				res = sizeof(int64_t);
				break;
			}
			return res;
		}

		template <class T>
		static constexpr Type GetType()
		{
			if (std::is_same<T, float>::value)	return Type::Float;
			if (std::is_same<T, double>::value)	return Type::Double;
			if (std::is_same<T, uint32_t>::value)	return Type::Uint32;
			if (std::is_same<T, int32_t>::value)	return Type::Int32;
			if (std::is_same<T, uint64_t>::value)	return Type::Uint64;
			if (std::is_same<T, int64_t>::value)	return Type::Int64;
			return Type::MAX_ENUM;
		}

	protected:
		
		std::string _name;
		Type _type = Type::MAX_ENUM;
		std::string _unit = {};

		std::vector<AbstractStatRecord *> _children_records = {};

	public:

		AbstractStatRecord(std::string const& name, Type type, std::string const& unit) :
			_name(name),
			_type(type),
			_unit(unit)
		{

		}

		virtual void computeAverage(size_t begin, size_t len) = 0;

		void computeAverage()
		{
			computeAverage(0, 0);
		}

		virtual void advance(size_t index) = 0;
		
		virtual void declareGui(GuiContext & ctx, size_t latest_index, bool show_graph, size_t depth = 0) = 0;

		constexpr Type type()const
		{
			return _type;
		}

		constexpr const std::string& name()const
		{
			return _name;
		}

		constexpr const std::string& unit()const
		{
			return _unit;
		}
	};

	template <class T, class Dec = double>
	class StatRecord : public AbstractStatRecord
	{
	protected:

		
		std::vector<T> _ring_buffer;
		T _sum = T(0);
		T _avg = T(0);
		Dec _scale = Dec(1);
		Dec _avg_dec = Dec(0);

		Dyn<T> _provider;

	public:

		struct CreateInfo
		{
			std::string name = {};
			size_t memory = 0;
			Dec scale = Dec(1);
			Dyn<T> provider = {};
			std::string unit = {};
		};
		using CI = CreateInfo;

		StatRecord(CreateInfo const& ci):
			AbstractStatRecord(ci.name, GetType<T>(), ci.unit),
			_ring_buffer(ci.memory, T(0)),
			_scale(ci.scale),
			_provider(ci.provider)
		{

		}

		virtual ~StatRecord()
		{
			for (auto& r : _children_records)
			{
				delete r;
				r = nullptr;
			}
		}

		virtual void computeAverage(size_t begin, size_t len) final override
		{
			_sum = T(0);
			size_t index = begin % _ring_buffer.size();
			for (size_t i = 0; i < len; ++i)
			{
				_sum += _ring_buffer[index];
				++index;
				if(index >= _ring_buffer.size())	index = 0;
			}

			_avg = _sum / len;
			_avg_dec = Dec(_sum) / Dec(len);

			for (auto& r : _children_records)
			{
				r->computeAverage(begin, len);
			}
		}

		virtual void advance(size_t i) final override
		{
			assert(i < _ring_buffer.size());
			if (_provider.hasValue())
			{
				_ring_buffer[i] = _provider.value();
			}
			else
			{
				_ring_buffer[i] = T(0);
			}

			for (auto& r : _children_records)
			{
				r->advance(i);
			}
		}

		void setRecord(size_t i, T const& t)
		{
			assert(i < _ring_buffer.size());
			_ring_buffer[i] = t;
		}

		void addRecord(size_t i, T const& t)
		{
			assert(i < _ring_buffer.size());
			_ring_buffer[i] += t;
		}

		void setProvider(Dyn<T> const& p)
		{
			_provider = p;
		}
		
		virtual void declareGui(GuiContext& ctx, size_t latest_index, bool show_graph, size_t depth) override
		{
			ImGui::PushID(name().c_str());
			
			float avg = _avg_dec * _scale;
			std::string text = name() + ": %.3f ";
			if (!_unit.empty())
			{
				text += _unit;
			}
			if (_children_records.empty())
			{
				ImGui::Text(text.c_str(), avg);
			}
			else
			{
				if (ImGui::TreeNode(name().c_str(), text.c_str(), avg))
				{
					for (auto& c : _children_records)
					{
						c->declareGui(ctx, latest_index, show_graph, depth + 1);
					}
					ImGui::TreePop();
				}
			}


			if (show_graph)
			{
				//ImGui::PlotLines()
				ImGui::Text("TODO");
			}

			ImGui::PopID();
		}

		template <class Q, class Dec2 = double>
		struct CreateRecordInfo
		{
			std::string name = {};
			Dec2 scale = Dec2(1);
			Dyn<Q> provider = {};
			std::string unit = {};
		};

		template <class Q = T, class Dec2 = Dec>
		using CRI = CreateRecordInfo<Q, Dec2>;

		template <class Q = T, class Dec2 = Dec>
		StatRecord<Q, Dec2>* createChildRecord(CreateRecordInfo<Q, Dec2> const& cri)
		{
			StatRecord<Q, Dec2>* res = new StatRecord<Q, Dec2>(typename StatRecord<Q, Dec2>::CreateInfo{
				.name = cri.name,
				.memory = _ring_buffer.size(),
				.scale = cri.scale,
				.provider = cri.provider,
				.unit = cri.unit,
			});
			_children_records.push_back(res);
			return res;
		}
	};

	class StatRecords
	{
	public:
		using Clock = std::chrono::high_resolution_clock;
	
	protected:
		
		std::string _name;
		size_t _memory;

		Clock::duration _period;
		Clock::time_point _latest_time_point;
		size_t _iter_counter_since_avg = 0;

		std::vector<AbstractStatRecord *> _records;

		bool _gui_show_graph = false;


		size_t _index = size_t(-1);

	public:

		struct CreateInfo
		{
			std::string name = {};
			size_t memory = 256;
			Clock::duration period = std::chrono::seconds(1);
		};
		using CI = CreateInfo;
		
		StatRecords(CreateInfo const& ci);

		~StatRecords();

		template <class T, class Dec = double>
		struct CreateRecordInfo
		{
			std::string name = {};
			Dec scale = Dec(1);
			Dyn<T> provider = {};
			std::string unit = {};
		};

		template <class T, class Dec = double>
		using CRI = CreateRecordInfo<T, Dec>;

		template <class T, class Dec = double>
		StatRecord<T, Dec> * createRecord(CreateRecordInfo<T, Dec> const& cri)
		{
			StatRecord<T, Dec>* res = new StatRecord<T, Dec>(typename StatRecord<T, Dec>::CI{
				.name = cri.name,
				.memory = _memory,
				.scale = cri.scale,
				.provider = cri.provider,
				.unit = cri.unit,
			});
			_records.push_back(res);
			return res;
		}

		void advance();

		void declareGui(GuiContext & ctx);

		constexpr const std::string& name()const
		{
			return _name;
		}

		constexpr size_t index()const
		{
			return _index;
		}

		void createCommonRecords(FramePerfCounters& fpc);
	};
}
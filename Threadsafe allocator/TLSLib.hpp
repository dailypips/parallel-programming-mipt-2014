#include <stdexcept>
#include <queue>
#include <list>
#include <cstdlib>
#include <set>
#include <atomic>
#include <memory>
#include <iostream>
#include <algorithm>
#include <mutex>

#include "malloc_allocator.hpp"

namespace tls
{
	// constants
	const size_t BIN_SIZES_COUNT = 6;
	const size_t MAX_BLOCK_SIZE = 256;

	// forward declarations
	class local_pool;
	class super_block_bin;

	union bin_t
	{
		bin_t * next;
		char * data;
	};

	enum _pool_state
	{
		ps_initializing,
		ps_ready,
		ps_destroyed
	};

	///////////////////////////////////////////////////////

	class super_block_t
	{
		struct super_block_header_t
		{
			bin_t * avail;
			std::queue<void *, std::deque<void *, malloc_allocator<void *>>> blocks_to_delete;
			size_t blocks_avail;
			size_t block_size;
			std::mutex mutex;
		};

		static const int SUPER_BLOCK_SIZE = 4096;
		static const int DATA_SIZE = SUPER_BLOCK_SIZE - sizeof(super_block_header_t);

		super_block_header_t header;
		char blocks[DATA_SIZE];

	public:
		super_block_t(size_t block_size);

		size_t avail_space() const
		{
			return header.blocks_avail;
		}

		size_t get_block_size() const
		{
			return header.block_size;
		}

		bool check_block_ownership(void * block) const
		{
			return (block >= blocks && block <= blocks + (DATA_SIZE - 1));
		}

		void add_block_for_delete(void * block)
		{
			std::lock_guard<std::mutex> lk(header.mutex);
			header.blocks_to_delete.push(block);
		}

		void update()
		{
			_delete_queued_blocks();
		}

		bool empty()
		{
			return avail_space() == _max_block_count();
		}

		void * new_block(size_t count);
		void free_block(void * block);

	private:
		size_t _max_block_count() const
		{
			return DATA_SIZE / header.block_size;
		}

		void _init();		
		void _free_block_no_check(void * block);
		void _delete_queued_blocks();
	};

	typedef super_block_t * _sb_ptr;
	typedef malloc_allocator<_sb_ptr> alloc;

	///////////////////////////////////////////////////////

	class super_block_cache
	{
	private:
		std::vector<_sb_ptr, alloc> blocks[BIN_SIZES_COUNT];
		size_t _get_index_for_size(size_t block_size);

	public:
		super_block_cache();
		~super_block_cache();
		void add_block(_sb_ptr block);
		_sb_ptr get_block(size_t block_size);
	};

	///////////////////////////////////////////////////////
	
	class main_pool
	{
	private:
		super_block_cache cache;
		std::set<_sb_ptr, std::less<_sb_ptr>, alloc> used_blocks;
		
		std::recursive_mutex mutex;

		typedef std::recursive_mutex _rmx;

	public:
		~main_pool();

		_sb_ptr new_super_block(size_t block_size);
		void free_super_block(_sb_ptr block);
		void init_local_pool(super_block_bin bins[]);
		void * new_big_block(size_t n_bytes);
		void free_foreign_mem(void * mem);

		
	};

	//////////////////////////////////////////////////////////

	class super_block_bin
	{
	private:
		std::vector<_sb_ptr, alloc> blocks;
		main_pool * pool;
		size_t block_size;

		_sb_ptr _get_block_avail_space();
		void _sort();

	public:
		bool check_ownership(void * mem);
		size_t get_block_size() const;
		void update_blocks();
		void * new_mem(size_t n_bytes);
		void free_mem(void * mem);
		void set_pool(main_pool * pool);
		void set_block_size(size_t block_size);
		void add_block(_sb_ptr block);
	};
	
	//////////////////////////////////////////////////////////

	class local_pool
	{
	private:
		super_block_bin bins[BIN_SIZES_COUNT];
		main_pool * pool;

		size_t _get_bin_for_size(size_t n_bytes);
		size_t _get_bin_for_mem(void * mem);
		void _update_blocks();

	public:
		local_pool(main_pool & pool);
		void * new_mem(size_t n_bytes);
		void free_mem(void * mem);
	};
}

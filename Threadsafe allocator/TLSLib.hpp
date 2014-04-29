#include <stdexcept>
#include <queue>
#include <list>
#include <cstdlib>
#include <set>
#include <atomic>
#include <memory>

#include "malloc_allocator.hpp"

namespace tls
{
	// constants
	const size_t BIN_SIZES_COUNT = 6;
	const size_t MAX_BLOCK_SIZE = 256;

	// forward declarations
	class local_pool;
	class super_block_bin;

	template<size_t SIZE>
	union bin_t
	{
		bin_t<SIZE> * next;
		char data[SIZE];
	};

	///////////////////////////////////////////////////////

	class super_block_base_t
	{
	public:
		virtual size_t get_block_size() const = 0;
		virtual void * new_block(size_t n_bytes) = 0;
		virtual void free_block(void * block) = 0;
		virtual bool check_block_ownership(void * block) const = 0;
		virtual void add_block_for_delete(void * block) = 0;
		virtual size_t avail_space() const = 0;
		virtual super_block_base_t * create_block() = 0;
		virtual void update() = 0;
		virtual bool empty() = 0;
	};

	typedef super_block_base_t * _sb_ptr;
	typedef malloc_allocator<_sb_ptr> alloc;

	///////////////////////////////////////////////////////

	template<size_t BIN_SIZE>
	class super_block_t : public super_block_base_t
	{
		typedef bin_t<BIN_SIZE> bin_sized_t;

		struct super_block_header_t
		{
			bin_sized_t * avail;
			std::queue<void *> blocks_to_delete;
			size_t blocks_avail;
		};

		static const int SUPER_BLOCK_SIZE = 4096;
		static const int EFFECTIVE_SIZE = SUPER_BLOCK_SIZE - sizeof(super_block_header_t);
		static const int MAX_BINS = EFFECTIVE_SIZE / BIN_SIZE;

		super_block_header_t header;
		bin_sized_t blocks[MAX_BINS];


	public:
		super_block_t();

		virtual size_t avail_space() const
		{
			return header.blocks_avail;
		}

		virtual size_t get_block_size() const
		{
			return BIN_SIZE;
		}

		virtual _sb_ptr create_block()
		{
			return new super_block_t<BIN_SIZE>();
		}

		virtual bool check_block_ownership(void * block) const
		{
			return (block >= blocks && block <= blocks + (MAX_BINS - 1));
		}

		virtual void add_block_for_delete(void * block)
		{
			header.blocks_to_delete.push(block);
		}

		virtual void update()
		{
			_delete_queued_blocks();
		}

		virtual bool empty()
		{
			return avail_space() == MAX_BINS;
		}

		virtual void * new_block(size_t count);
		virtual void free_block(void * block);

	protected:
		virtual size_t get_bin_size() const
		{
			return BIN_SIZE;
		}

	private:
		void _init();
		void _free_block_no_check(void * block);
		void _delete_queued_blocks();
	};

	///////////////////////////////////////////////////////

	class super_block_cache
	{
	private:
		std::vector<_sb_ptr, alloc> blocks[BIN_SIZES_COUNT];
		size_t _get_index_for_size(size_t block_size);

	public:
		super_block_cache();
		void add_block(_sb_ptr block);
		_sb_ptr get_block(size_t block_size);
	};

	///////////////////////////////////////////////////////
	
	class main_pool
	{
	private:
		static size_t bin_sizes[BIN_SIZES_COUNT];

		super_block_cache cache;
		std::set<_sb_ptr, std::less<_sb_ptr>, alloc> used_blocks;

		//_sb_ptr _get_block_for_mem(void * mem);

	public:
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

		_sb_ptr _get_block_avail_space();
		void _sort();

	public:
		bool check_ownership(void * mem);
		size_t get_block_size() const;
		void update_blocks();
		void * new_mem(size_t n_bytes);
		void free_mem(void * mem);
		void set_pool(main_pool * pool);
		void add_block(_sb_ptr block);
	};
	
	//////////////////////////////////////////////////////////

	class local_pool
	{
	private:
		bool is_initializing;
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
#include <stdexcept>
#include <queue>
#include <list>
#include <cstdlib>
#include <set>

namespace tls
{
	// constants
	const size_t BIN_SIZES_COUNT = 6;
	const size_t MAX_BLOCK_SIZE = 256;

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
	};

	typedef super_block_base_t * _sb_ptr;

	///////////////////////////////////////////////////////

	template<size_t BIN_SIZE>
	class super_block_t : public super_block_base_t
	{
		typedef bin_t<BIN_SIZE> bin_sized_t;

		struct super_block_header_t
		{
			size_t id;
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

	class super_blocks_bin
	{
	private:
		std::vector<_sb_ptr> blocks; // Храним блоки по убыванию занятого места
		main_block_pool * pool;

		void _sort();
	public:
		void add_block(_sb_ptr block);
		size_t get_block_size() const;
		_sb_ptr get_block_space_avail() const;
		bool check_ownership(void * mem) const;
		void set_main_pool(main_block_pool * pool);
		void free_mem(void * mem);
		void * new_mem(size_t n_bytes);
	};

	///////////////////////////////////////////////////////

	class super_block_storage
	{
	private:
		std::queue<_sb_ptr> blocks;
	public:
		void add_block(_sb_ptr block);
		_sb_ptr get_block();
	};

	///////////////////////////////////////////////////////

	class main_block_pool
	{
	public:
		main_block_pool();
		void init_local_pool(super_blocks_bin local_blocks[]);
		void * get_big_block(size_t n_bytes);
		_sb_ptr get_super_block(size_t bin_size);
		void free_super_block(_sb_ptr block);

		void free_foreign_block(void * mem);

	private:
		static size_t bin_sizes[BIN_SIZES_COUNT];
		super_block_storage blocks[BIN_SIZES_COUNT];
		std::set<_sb_ptr> used_blocks;

		size_t _get_index_for_size(size_t bin_size) const;
	};

	///////////////////////////////////////////////////////

	class local_block_pool
	{
	private:
		main_block_pool & main_pool;
		super_blocks_bin blocks[BIN_SIZES_COUNT];

		size_t _get_bin_for_size(size_t n_bytes) const;
		size_t _get_bin_for_mem(void * mem) const;

	public:
		local_block_pool(main_block_pool & pool) :
			main_pool(pool)
		{
			main_pool.init_local_pool(blocks);
		}

		void * new_block(size_t n_bytes);
		void free_block(void * mem);
	};

}
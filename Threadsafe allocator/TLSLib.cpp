#include "TLSLib.hpp"

namespace tls
{
	size_t main_pool::bin_sizes[] = { 8, 16, 32, 64, 128, 256 };

	///////////////////////////////////////////////////////
	// class super_block_t
	///////////////////////////////////////////////////////

	template<size_t BIN_SIZE>
	super_block_t<BIN_SIZE>::super_block_t()
	{
		_init();
		header.blocks_avail = MAX_BINS;
	}

	template<size_t BIN_SIZE>
	void * super_block_t<BIN_SIZE>::new_block(size_t count)
	{
		if (avail_space() == 0)
		{
			throw std::exception("out of blocks");
		}

		if (count > BIN_SIZE)
		{
			throw std::invalid_argument("incorrect block size");
		}

		void * result = header.avail;
		header.avail = header.avail->next;
		header.blocks_avail--;
		return result;
	}

	template<size_t BIN_SIZE>
	void super_block_t<BIN_SIZE>::free_block(void * block)
	{
		if (block == nullptr)
		{
			return;
		}

		if (!check_block_ownership(block))
		{
			throw std::invalid_argument("foreign block!");
		}

		_free_block_no_check(block);
		_delete_queued_blocks();
	}

	template<size_t BIN_SIZE>
	void super_block_t<BIN_SIZE>::_init()
	{
		for (size_t index = 1; index < MAX_BINS; ++index)
		{
			blocks[index - 1].next = &blocks[index];
		}
		blocks[MAX_BINS - 1].next = nullptr;
		header.avail = blocks;
	}

	template<size_t BIN_SIZE>
	void super_block_t<BIN_SIZE>::_free_block_no_check(void * block)
	{
		bin_sized_t * bin_block = (bin_sized_t *)block;
		bin_block->next = header.avail;
		header.avail = bin_block;
		header.blocks_avail++;
	}

	template<size_t BIN_SIZE>
	void super_block_t<BIN_SIZE>::_delete_queued_blocks()
	{
		while (!header.blocks_to_delete.empty())
		{
			void * block = header.blocks_to_delete.front();
			header.blocks_to_delete.pop();
			_free_block_no_check(block);
		}
	}

	///////////////////////////////////////////////////////
	// class super_block_bin
	///////////////////////////////////////////////////////
	bool super_block_bin::check_ownership(void * mem)
	{
		for (auto & it : blocks)
		{
			if (it->check_block_ownership(mem))
			{
				return true;
			}
		}
		return false;
	}

	void super_block_bin::update_blocks()
	{
		for (auto & it : blocks)
		{
			it->update();
		}

		_sort();
	}

	size_t super_block_bin::get_block_size() const
	{
		return blocks.front()->get_block_size();
	}

	void * super_block_bin::new_mem(size_t n_bytes)
	{
		_sb_ptr block = _get_block_avail_space();
		if (block == nullptr)
		{
			size_t block_size = get_block_size();
			block = pool->new_super_block(block_size);
			blocks.push_back(block);
		}

		return block->new_block(n_bytes);
	}

	void super_block_bin::free_mem(void * mem)
	{
		for (auto & it : blocks)
		{
			if (it->check_block_ownership(mem))
			{
				it->free_block(mem);
			}
		}
	}

	_sb_ptr super_block_bin::_get_block_avail_space()
	{
		for (auto & it : blocks)
		{
			if (it->avail_space() != 0)
			{
				return it;
			}
		}

		return nullptr;
	}

	void super_block_bin::_sort()
	{
		auto cmp = [](const _sb_ptr first, const _sb_ptr second)
		{
			return first->avail_space() < second->avail_space();
		};

		std::sort(blocks.begin(), blocks.end(), cmp);
	}

	void super_block_bin::set_pool(main_pool * pool)
	{
		this->pool = pool;
	}

	void super_block_bin::add_block(_sb_ptr block)
	{
		blocks.push_back(block);
	}

	///////////////////////////////////////////////////////
	// class local_pool
	///////////////////////////////////////////////////////
	local_pool::local_pool(main_pool & pool) :
		is_initializing(true)
	{
		this->pool = &pool;
		for (size_t index = 0; index < BIN_SIZES_COUNT; ++index)
		{
			bins[index].set_pool(&pool);
		}

		pool.init_local_pool(bins);
		is_initializing = false;
	}

	void * local_pool::new_mem(size_t n_bytes)
	{
		if (is_initializing)
		{
			return pool->new_big_block(n_bytes);
		}

		size_t index = _get_bin_for_size(n_bytes);
		if (index == -1)
		{
			return pool->new_big_block(n_bytes);
		}

		_update_blocks();

		return bins[index].new_mem(n_bytes);
	}

	void local_pool::free_mem(void * mem)
	{
		size_t index = _get_bin_for_mem(mem);
		if (index == -1)
		{
			pool->free_foreign_mem(mem);
		}

		_update_blocks();

		bins[index].free_mem(mem);
	}

	size_t local_pool::_get_bin_for_mem(void * mem)
	{
		for (size_t index = 0; index < BIN_SIZES_COUNT; ++index)
		{
			if (bins[index].check_ownership(mem))
			{
				return index;
			}
		}

		return -1;
	}

	size_t local_pool::_get_bin_for_size(size_t n_bytes)
	{
		for (size_t index = 0; index < BIN_SIZES_COUNT; ++index)
		{
			if (bins[index].get_block_size() > n_bytes)
			{
				return index;
			}
		}

		return -1;
	}

	void local_pool::_update_blocks()
	{
		for (size_t index = 0; index < BIN_SIZES_COUNT; ++index)
		{
			bins[index].update_blocks();
		}
	}

	///////////////////////////////////////////////////////
	// class super_block_cache
	///////////////////////////////////////////////////////
	void super_block_cache::add_block(_sb_ptr block)
	{
		size_t index = _get_index_for_size(block->get_block_size());
		if (index == -1)
		{
			throw std::invalid_argument("unknown block size");
		}

		blocks[index].push_back(block);
	}

	_sb_ptr super_block_cache::get_block(size_t block_size)
	{
		size_t index = _get_index_for_size(block_size);
		if (index == -1)
		{
			throw std::invalid_argument("unknown block size");
		}

		auto res = blocks[index].back();
		blocks[index].pop_back();
		if (blocks[index].size() == 0)
		{
			blocks[index].push_back(res->create_block());
		}

		return res;
	}

	size_t super_block_cache::_get_index_for_size(size_t block_size)
	{
		for (size_t index = 0; index < BIN_SIZES_COUNT; ++index)
		{
			if (blocks[index].front()->get_block_size() == block_size)
			{
				return index;
			}
		}

		return -1;
	}

	super_block_cache::super_block_cache()
	{
		void * p = malloc(sizeof(super_block_t<8>));
		blocks[0].push_back(new(p) super_block_t<8>());

		p = malloc(sizeof(super_block_t<16>));
		blocks[1].push_back(new(p)super_block_t<16>());

		p = malloc(sizeof(super_block_t<32>));
		blocks[2].push_back(new(p)super_block_t<32>());

		p = malloc(sizeof(super_block_t<64>));
		blocks[3].push_back(new(p)super_block_t<64>());

		p = malloc(sizeof(super_block_t<128>));
		blocks[4].push_back(new(p)super_block_t<128>());

		p = malloc(sizeof(super_block_t<256>));
		blocks[5].push_back(new(p)super_block_t<256>());
	}

	///////////////////////////////////////////////////////
	// class main_pool
	///////////////////////////////////////////////////////
	_sb_ptr main_pool::new_super_block(size_t block_size)
	{
		auto result = cache.get_block(block_size);
		used_blocks.insert(result);
		return result;
	}

	void main_pool::free_super_block(_sb_ptr block)
	{
		auto it = used_blocks.find(block);
		if (it == used_blocks.end())
		{
			throw std::invalid_argument("foreign block");
		}

		used_blocks.erase(it);
		cache.add_block(block);
	}

	void * main_pool::new_big_block(size_t n_bytes)
	{
		return malloc(n_bytes);
	}

	void main_pool::free_foreign_mem(void * mem)
	{
		for (auto & it : used_blocks)
		{
			if (it->check_block_ownership(mem))
			{
				it->add_block_for_delete(mem);
				return;
			}
		}

		free(mem);
	}

	void main_pool::init_local_pool(super_block_bin bins[])
	{
		for (size_t index = 0; index < BIN_SIZES_COUNT; ++index)
		{
			bins[index].add_block(new_super_block(bin_sizes[index]));
		}
	}
}
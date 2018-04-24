#include <gtest/gtest.h>
#include "q/spsc.hpp"
#include "q/mpmc.hpp"
#include "q/q_api.hpp"
#include <string>

using namespace std;
using Type = string;
using FlexibleQ = spsc::flexible::circular_fifo<Type>;
using FixedQ = spsc::fixed::circular_fifo<Type, 100>;
using FixedSmallQ = spsc::fixed::circular_fifo<Type, 2>;
using LockedQ = mpmc::dynamic_lock_queue<Type>;



template<typename Prod, typename Cons>
void ProdConsInitialization(Prod& prod, Cons& cons ) {
   EXPECT_TRUE(prod.empty());
   EXPECT_TRUE(cons.empty());

   EXPECT_FALSE(prod.full());
   EXPECT_FALSE(cons.full());

   EXPECT_TRUE(prod.lock_free());
   EXPECT_TRUE(cons.lock_free());


   EXPECT_EQ(10, prod.capacity());
   EXPECT_EQ(10, cons.capacity());

   EXPECT_EQ(10, prod.capacity_free());
   EXPECT_EQ(10, cons.capacity_free());

   EXPECT_EQ(0, prod.size());
   EXPECT_EQ(0, cons.size());
}


TEST(Queue, ProdConsInitialization) {
   auto queue = queue_api::CreateQueue<FlexibleQ>(10);
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
}


template<typename Prod>
void QAddOne(Prod& prod) {
   std::string arg = "test";
   EXPECT_TRUE(prod.push(arg));
   EXPECT_FALSE(prod.full());
   EXPECT_EQ(100, prod.capacity());
   EXPECT_EQ(99, prod.capacity_free());
   EXPECT_EQ(1, prod.size());
   EXPECT_EQ(1, prod.tail());
}

TEST(Queue, CircularQueue_AddOne) {
   FlexibleQ dQ{100};
   QAddOne(dQ);

   FixedQ fQ{};
   QAddOne(fQ);

}


template<typename Prod, typename Cons>
void AddTillFullRemoveTillEmpty(Prod& prod, Cons& cons) {
   int size = 0;
   int free = prod._qref.capacity();
   int loopSize = 2;

   EXPECT_EQ(0, prod.usage());
   for (size_t i = 0; i < loopSize; ++i) {
      while (!prod.full()) {
         std::string value = to_string(i);
         EXPECT_TRUE(prod.push(value));
         ++size;
         --free;
         EXPECT_EQ((100 * prod._qref.size() / prod._qref.capacity()), prod._qref.usage());
         EXPECT_EQ(size, prod.size());
         EXPECT_EQ(free, prod.capacity_free());
      }
      EXPECT_TRUE(prod.full());
      EXPECT_TRUE(cons.full());
      EXPECT_EQ(100, prod.usage());
      EXPECT_EQ(prod.size(), prod.capacity()) << "i: " << i;
      EXPECT_EQ(cons.size(), cons.capacity());

      string t;
      while (!cons.empty()) {
         cons.pop(t);
         --size;
         ++free;
         EXPECT_EQ((100 * prod._qref.size() / prod._qref.capacity()), prod._qref.usage());
         EXPECT_EQ(size, cons.size());
         EXPECT_EQ(free, cons.capacity_free());
      }
   }
}



TEST(Queue, FlexibleQueue_AddTillFullRemoveTillEmpty) {
   auto queue = queue_api::CreateQueue<FlexibleQ>(100);
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   AddTillFullRemoveTillEmpty(producer, consumer);
}

TEST(Queue, FixedQueue_AddTillFullRemoveTillEmpty) {
   auto queue = queue_api::CreateQueue<FixedQ>();
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   AddTillFullRemoveTillEmpty(producer, consumer);
}

TEST(Queue, LockedQ_AddTillFullRemoveTillEmpty) {
   auto queue = queue_api::CreateQueue<LockedQ>(100);
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   AddTillFullRemoveTillEmpty(producer, consumer);
}


template<typename Prod, typename Cons>
void MoveArgument(Prod& prod, Cons& cons) {
   std::string arg = "hello";
   EXPECT_TRUE(prod.push(arg));
   EXPECT_TRUE(arg.empty());

   arg = "world";
   EXPECT_TRUE(prod.push(arg));
   EXPECT_TRUE(arg.empty());

   arg = "!";
   EXPECT_FALSE(prod.push(arg));
   EXPECT_FALSE(arg.empty());
   EXPECT_EQ("!", arg);

   EXPECT_TRUE(cons.pop(arg));
   EXPECT_EQ("hello", arg);
}

TEST(Queue, FlexibleQ_MoveArgument) {
   auto queue = queue_api::CreateQueue<FlexibleQ>(2);
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   MoveArgument(producer, consumer);
}

TEST(Queue, FixedSmallQ_MoveArgument) {
   auto queue = queue_api::CreateQueue<FixedSmallQ>();
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   MoveArgument(producer, consumer);
}

TEST(Queue, LockedQ_MoveArgument) {
   auto queue = queue_api::CreateQueue<LockedQ>(2);
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   MoveArgument(producer, consumer);
}



template<typename Prod, typename Cons>
void MoveUniquePtrArgument(Prod& prod, Cons& cons) {
   auto arg = make_unique<string>("hello");
   EXPECT_TRUE(prod.push(arg));
   ASSERT_TRUE(nullptr == arg);

   arg = make_unique<string>("world");
   EXPECT_TRUE(prod.push(arg));
   ASSERT_TRUE(nullptr == arg);

   arg = make_unique<string>("!");
   EXPECT_FALSE(prod.push(arg));
   EXPECT_FALSE(arg->empty());
   ASSERT_FALSE(nullptr == arg);
   EXPECT_EQ("!", *arg.get());

   EXPECT_TRUE(cons.pop(arg));
   EXPECT_EQ("hello", *arg.get());
}

namespace {
   using Unique = unique_ptr<string>;
}

TEST(Queue, FlexibleQ_MoveUnique) {
   auto queue = queue_api::CreateQueue<spsc::flexible::circular_fifo<Unique>>(2);
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   MoveUniquePtrArgument(producer, consumer);
}

TEST(Queue, FixedQ_MoveUnique) {
   auto queue = queue_api::CreateQueue<spsc::fixed::circular_fifo<Unique, 2>>();
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   MoveUniquePtrArgument(producer, consumer);
}

TEST(Queue, LockedQ_MoveUnique) {
   auto queue = queue_api::CreateQueue<mpmc::dynamic_lock_queue<Unique>>(2);
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   MoveUniquePtrArgument(producer, consumer);
}


// Raw pointers are not transformed to nullptr
// since move semantics won't affect them
template<typename Prod, typename Cons>
void NoMovePtrArgument(Prod& prod, Cons& cons) {
   auto arg1 = make_unique<string>("hello");
   auto arg1ptr = arg1.get();
   EXPECT_TRUE(prod.push(arg1ptr));
   ASSERT_FALSE(nullptr == arg1ptr);

   auto arg2 = make_unique<string>("world");
   auto arg2ptr = arg2.get();
   EXPECT_TRUE(prod.push(arg2ptr));
   ASSERT_FALSE(nullptr == arg2ptr);

   auto arg3 = make_unique<string>("!");
   auto arg3ptr = arg3.get();
   EXPECT_FALSE(prod.push(arg3ptr));
   EXPECT_FALSE(arg3ptr->empty());
   ASSERT_FALSE(nullptr == arg3ptr);
   EXPECT_EQ("!", *arg3ptr);

   string* receive = nullptr;
   EXPECT_TRUE(cons.pop(receive));
   EXPECT_EQ("hello", *receive);
}

namespace {
   using Ptr = string*;
}


TEST(Queue, FlexibleQ_NoMoveOfPtr) {
   auto queue = queue_api::CreateQueue<spsc::flexible::circular_fifo<Ptr>>(2);
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   NoMovePtrArgument(producer, consumer);
}

TEST(Queue, FixedQ_NoMoveOfPtr) {
   auto queue = queue_api::CreateQueue<spsc::fixed::circular_fifo<Ptr, 2>>();
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   NoMovePtrArgument(producer, consumer);
}

TEST(Queue, LockedQ_MpMoveUnique) {
   auto queue = queue_api::CreateQueue<mpmc::dynamic_lock_queue<Ptr>>(2);
   auto producer = std::get<queue_api::index::sender>(queue);
   auto consumer = std::get<queue_api::index::receiver>(queue);
   NoMovePtrArgument(producer, consumer);
}

#include "isekai/host/falcon/falcon_resource_credits.h"

#include <type_traits>

#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "isekai/common/config.pb.h"

namespace isekai {
namespace {

const char kConfigCredits[] =
    R"pb(
  tx_packet_credits { ulp_requests: 1 ulp_data: 2 network_requests: 3 }
  tx_buffer_credits { ulp_requests: 4 ulp_data: 5 network_requests: 6 }
  rx_packet_credits { ulp_requests: 7 network_requests: 8 }
  rx_buffer_credits { ulp_requests: 9 network_requests: 10 })pb";

const char kConfigCreditsOversubscribed[] =
    R"pb(
  tx_packet_credits { ulp_requests: 1 ulp_data: 2 network_requests: 3 }
  tx_buffer_credits { ulp_requests: 4 ulp_data: 5 network_requests: 6 }
  rx_packet_credits { ulp_requests: 7 network_requests: 8 }
  rx_buffer_credits { ulp_requests: 9 network_requests: 10 }
  enable_ulp_pool_oversubscription: true)pb";

const FalconResourceCredits kCredits = {
    .tx_packet_credits =
        {
            .ulp_requests = 1,
            .ulp_data = 2,
            .network_requests = 3,
        },
    .tx_buffer_credits =
        {
            .ulp_requests = 4,
            .ulp_data = 5,
            .network_requests = 6,
        },
    .rx_packet_credits =
        {
            .ulp_requests = 7,
            .network_requests = 8,
        },
    .rx_buffer_credits =
        {
            .ulp_requests = 9,
            .network_requests = 10,
        },
};

const FalconResourceCredits kCreditsOversubscribed = {
    .tx_packet_credits =
        {
            .ulp_requests = 1,
            .ulp_data = 2,
            .network_requests = 3,
            .max_ulp_requests = 1,
        },
    .tx_buffer_credits =
        {
            .ulp_requests = 4,
            .ulp_data = 5,
            .network_requests = 6,
            .max_ulp_requests = 4,
        },
    .rx_packet_credits =
        {
            .ulp_requests = 7,
            .network_requests = 8,
        },
    .rx_buffer_credits =
        {
            .ulp_requests = 9,
            .network_requests = 10,
        },
    .enable_ulp_pool_oversubscription = true,
};

TEST(FalconResourceCredits, Create) {
  FalconConfig::ResourceCredits config_credits;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(kConfigCredits,
                                                            &config_credits));
  ASSERT_EQ(config_credits.tx_packet_credits().ulp_requests(), 1);
  ASSERT_EQ(config_credits.tx_packet_credits().ulp_data(), 2);
  ASSERT_EQ(config_credits.tx_packet_credits().network_requests(), 3);
  ASSERT_EQ(config_credits.tx_buffer_credits().ulp_requests(), 4);
  ASSERT_EQ(config_credits.tx_buffer_credits().ulp_data(), 5);
  ASSERT_EQ(config_credits.tx_buffer_credits().network_requests(), 6);
  ASSERT_EQ(config_credits.rx_packet_credits().ulp_requests(), 7);
  ASSERT_EQ(config_credits.rx_packet_credits().network_requests(), 8);
  ASSERT_EQ(config_credits.rx_buffer_credits().ulp_requests(), 9);
  ASSERT_EQ(config_credits.rx_buffer_credits().network_requests(), 10);

  FalconResourceCredits credits = FalconResourceCredits::Create(config_credits);
  EXPECT_EQ(credits, kCredits);
}

TEST(FalconResourceCredits, CreateOversubscribed) {
  FalconConfig::ResourceCredits config_credits;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      kConfigCreditsOversubscribed, &config_credits));
  ASSERT_EQ(config_credits.tx_packet_credits().ulp_requests(), 1);
  ASSERT_EQ(config_credits.tx_packet_credits().ulp_data(), 2);
  ASSERT_EQ(config_credits.tx_packet_credits().network_requests(), 3);
  ASSERT_EQ(config_credits.tx_buffer_credits().ulp_requests(), 4);
  ASSERT_EQ(config_credits.tx_buffer_credits().ulp_data(), 5);
  ASSERT_EQ(config_credits.tx_buffer_credits().network_requests(), 6);
  ASSERT_EQ(config_credits.rx_packet_credits().ulp_requests(), 7);
  ASSERT_EQ(config_credits.rx_packet_credits().network_requests(), 8);
  ASSERT_EQ(config_credits.rx_buffer_credits().ulp_requests(), 9);
  ASSERT_EQ(config_credits.rx_buffer_credits().network_requests(), 10);
  ASSERT_EQ(config_credits.enable_ulp_pool_oversubscription(), true);

  FalconResourceCredits credits = FalconResourceCredits::Create(config_credits);
  EXPECT_EQ(credits, kCreditsOversubscribed);
}

TEST(FalconResourceCredits, CompareEq) {
  const FalconResourceCredits eq = {
      .tx_packet_credits =
          {
              .ulp_requests = 1,
              .ulp_data = 2,
              .network_requests = 3,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 4,
              .ulp_data = 5,
              .network_requests = 6,
          },
      .rx_packet_credits =
          {
              .ulp_requests = 7,
              .network_requests = 8,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 9,
              .network_requests = 10,
          },
  };
  EXPECT_TRUE(eq <= kCredits);
  EXPECT_TRUE(eq == kCredits);
}

TEST(FalconResourceCredits, Compare2) {
  const FalconResourceCredits lt1 = {
      .tx_packet_credits =
          {
              .ulp_requests = 0,  // -1
              .ulp_data = 2,
              .network_requests = 3,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 4,
              .ulp_data = 5,
              .network_requests = 6,
          },
      .rx_packet_credits =
          {
              .ulp_requests = 7,
              .network_requests = 8,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 9,
              .network_requests = 10,
          },
  };
  EXPECT_TRUE(lt1 <= kCredits);
  EXPECT_FALSE(lt1 == kCredits);
}

TEST(FalconResourceCredits, Compare3) {
  const FalconResourceCredits lt2 = {
      .tx_packet_credits =
          {
              .ulp_requests = 1,
              .ulp_data = 2,
              .network_requests = 3,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 4,
              .ulp_data = 5,
              .network_requests = 4,  // -2
          },
      .rx_packet_credits =
          {
              .ulp_requests = 7,
              .network_requests = 8,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 9,
              .network_requests = 10,
          },
  };
  EXPECT_TRUE(lt2 <= kCredits);
  EXPECT_FALSE(lt2 == kCredits);
}

TEST(FalconResourceCredits, Compare4) {
  const FalconResourceCredits gt1 = {
      .tx_packet_credits =
          {
              .ulp_requests = 0,  // -1
              .ulp_data = 2,
              .network_requests = 3,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 4,
              .ulp_data = 5,
              .network_requests = 6,
          },
      .rx_packet_credits =
          {
              .ulp_requests = 8,  // +1
              .network_requests = 8,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 9,
              .network_requests = 10,
          },
  };
  EXPECT_FALSE(gt1 <= kCredits);
  EXPECT_FALSE(gt1 == kCredits);
}

TEST(FalconResourceCredits, Compare5) {
  const FalconResourceCredits gt2 = {
      .tx_packet_credits =
          {
              .ulp_requests = 1,
              .ulp_data = 2,
              .network_requests = 3,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 4,
              .ulp_data = 5,
              .network_requests = 9,  // +3
          },
      .rx_packet_credits =
          {
              .ulp_requests = 7,
              .network_requests = 7,  // -1
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 9,
              .network_requests = 10,
          },
  };
  EXPECT_FALSE(gt2 <= kCredits);
  EXPECT_FALSE(gt2 == kCredits);
}

TEST(FalconResourceCredits, PlusEqualEmpty) {
  FalconResourceCredits credits = kCredits;
  credits += {
      .tx_packet_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 0,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 0,
          },
      .rx_packet_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
  };
  EXPECT_EQ(credits, kCredits);
}

TEST(FalconResourceCredits, MinusEqualEmpty) {
  FalconResourceCredits credits = kCredits;
  credits -= {
      .tx_packet_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 0,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 0,
          },
      .rx_packet_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
  };
  EXPECT_EQ(credits, kCredits);
}

TEST(FalconResourceCredits, PlusEqualTwoFields) {
  FalconResourceCredits credits = kCredits;
  credits += {
      .tx_packet_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 2,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 0,
          },
      .rx_packet_credits =
          {
              .ulp_requests = 3,
              .network_requests = 0,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
  };
  FalconResourceCredits expected = kCredits;
  expected.tx_packet_credits.network_requests += 2;
  expected.rx_packet_credits.ulp_requests += 3;
  EXPECT_EQ(credits, expected);
}

TEST(FalconResourceCredits, MinusEqualTwoFields) {
  FalconResourceCredits credits = kCredits;
  credits -= {
      .tx_packet_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 2,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 0,
          },
      .rx_packet_credits =
          {
              .ulp_requests = 3,
              .network_requests = 0,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
  };
  FalconResourceCredits expected = kCredits;
  expected.tx_packet_credits.network_requests -= 2;
  expected.rx_packet_credits.ulp_requests -= 3;
  EXPECT_EQ(credits, expected);
}

TEST(FalconResourceCredits, CompareOversubscribed_TxPacket) {
  FalconResourceCredits credits = {
      .tx_packet_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 3,  // This is more than 2, but oversubscription lets
                              // us consume 1 from ulp_requests.
              .network_requests = 0,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 0,
          },
      .rx_packet_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
  };
  EXPECT_TRUE(credits <= kCreditsOversubscribed);

  // Set ulp_data to more than ulp_requests (1) + ulp_data (2), it should fail.
  credits.tx_packet_credits.ulp_data = 4;
  EXPECT_FALSE(credits <= kCreditsOversubscribed);

  // Set ulp_request to 1 and ulp_data to 3, such that the total is greater than
  // ulp_requests + ulp_data, it should fail.
  credits.tx_packet_credits.ulp_requests = 1;
  credits.tx_packet_credits.ulp_data = 3;
  EXPECT_FALSE(credits <= kCreditsOversubscribed);
}

TEST(FalconResourceCredits, CompareOversubscribed_Buffer) {
  FalconResourceCredits credits = {
      .tx_packet_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 0,
              .network_requests = 0,
          },
      .tx_buffer_credits =
          {
              .ulp_requests = 0,
              .ulp_data = 9,  // This is more than 5, but oversubscription lets
                              // us consume upto 4 from ulp_requests.
              .network_requests = 0,
          },
      .rx_packet_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
      .rx_buffer_credits =
          {
              .ulp_requests = 0,
              .network_requests = 0,
          },
      .enable_ulp_pool_oversubscription = true,
  };
  EXPECT_TRUE(credits <= kCreditsOversubscribed);

  // Set ulp_data to more than ulp_requests (4) + ulp_data (5), it should fail.
  credits.tx_buffer_credits.ulp_data = 10;
  EXPECT_FALSE(credits <= kCreditsOversubscribed);

  // Set ulp_request to 2 and ulp_data to 7, such that the total is less than
  // ulp_requests + ulp_data, it should succeed.
  credits.tx_buffer_credits.ulp_requests = 2;
  credits.tx_buffer_credits.ulp_data = 7;
  EXPECT_TRUE(credits <= kCreditsOversubscribed);

  // Set ulp_request to 2 and ulp_data to 8, such that the total is greater than
  // ulp_requests + ulp_data, it should fail.
  credits.tx_buffer_credits.ulp_requests = 2;
  credits.tx_buffer_credits.ulp_data = 8;
  EXPECT_FALSE(credits <= kCreditsOversubscribed);
}

TEST(FalconResourceCredits, AddSubtractOversubscribed_Packet) {
  FalconResourceCredits credits = kCreditsOversubscribed;

  FalconResourceCredits rhs_0;           // Defaults to all 0 credits.
  rhs_0.tx_packet_credits.ulp_data = 3;  // Sum of credits.request + data.
  EXPECT_TRUE(rhs_0 <= credits);
  credits -= rhs_0;
  // Both requests and data credits should be 0 due to oversubscription.
  EXPECT_EQ(credits.tx_packet_credits.ulp_requests, 0);
  EXPECT_EQ(credits.tx_packet_credits.ulp_data, 0);

  // Return single ulp_data credit, it should increment request pool.
  FalconResourceCredits rhs_1;
  rhs_1.tx_packet_credits.ulp_data = 1;
  credits += rhs_1;
  EXPECT_EQ(credits.tx_packet_credits.ulp_requests, 1);
  EXPECT_EQ(credits.tx_packet_credits.ulp_data, 0);

  // Return another single ulp_data credit, it should increment data pool since
  // max_ulp_request is set to 1.
  credits += rhs_1;
  EXPECT_EQ(credits.tx_packet_credits.ulp_requests, 1);
  EXPECT_EQ(credits.tx_packet_credits.ulp_data, 1);
}

TEST(FalconResourceCredits, AddSubtractOversubscribed_Buffer) {
  FalconResourceCredits credits = kCreditsOversubscribed;

  FalconResourceCredits rhs_0;           // Defaults to all 0 credits.
  rhs_0.tx_buffer_credits.ulp_data = 9;  // Sum of credits.request + data.
  EXPECT_TRUE(rhs_0 <= credits);
  credits -= rhs_0;
  // Both requests and data credits should be 0 due to oversubscription.
  EXPECT_EQ(credits.tx_buffer_credits.ulp_requests, 0);
  EXPECT_EQ(credits.tx_buffer_credits.ulp_data, 0);

  // Return single ulp_data credit, it should increment request pool.
  FalconResourceCredits rhs_1;
  rhs_1.tx_buffer_credits.ulp_data = 1;
  credits += rhs_1;
  EXPECT_EQ(credits.tx_buffer_credits.ulp_requests, 1);
  EXPECT_EQ(credits.tx_buffer_credits.ulp_data, 0);

  // Return 5 ulp_data credit, it should increment request to max_request (4)
  // and increment data with remaining credits.
  FalconResourceCredits rhs_2;
  rhs_2.tx_buffer_credits.ulp_data = 5;
  credits += rhs_2;
  EXPECT_EQ(credits.tx_buffer_credits.ulp_requests, 4);
  EXPECT_EQ(credits.tx_buffer_credits.ulp_data, 2);
}

}  // namespace
}  // namespace isekai

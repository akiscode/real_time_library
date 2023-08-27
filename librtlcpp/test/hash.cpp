// Copyright (c) 2023. Akiscode
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rtlcpp/hash.hpp"

#include <gtest/gtest.h>

class HashTest : public ::testing::Test {
 protected:
  HashTest() {
    // You can do set-up work for each test here.
  }

  ~HashTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }
};

TEST_F(HashTest, FNV1A_str_Test) {
  ASSERT_EQ(rtl::fnv1a("TestStr"), 2192168560);
  ASSERT_EQ(rtl::hash<const char *>{}("TestStr"), 2192168560);
  ASSERT_EQ(rtl::hash<std::string>{}("TestStr"), 2192168560);

  ASSERT_EQ(rtl::fnv1a("http://akiscode.com"), 3687397249);
  ASSERT_EQ(rtl::hash<const char *>{}("http://akiscode.com"), 3687397249);
  ASSERT_EQ(rtl::hash<std::string>{}("http://akiscode.com"), 3687397249);

  ASSERT_EQ(rtl::fnv1a("1289139asdf9a89uasd9fajsdf9asdfa0923091203"),
            3018378392);
  ASSERT_EQ(
      rtl::hash<const char *>{}("1289139asdf9a89uasd9fajsdf9asdfa0923091203"),
      3018378392);
  ASSERT_EQ(
      rtl::hash<std::string>{}("1289139asdf9a89uasd9fajsdf9asdfa0923091203"),
      3018378392);
}

TEST_F(HashTest, FNV1A_uint8_Test) {
  ASSERT_EQ(rtl::fnv1a((uint8_t)1), 67918732);
  ASSERT_EQ(rtl::hash<uint8_t>{}(1), 67918732);
  ASSERT_EQ(rtl::fnv1a((uint8_t)2), 118251589);
  ASSERT_EQ(rtl::hash<uint8_t>{}(2), 118251589);
  ASSERT_EQ(rtl::fnv1a((uint8_t)3), 101473970);
  ASSERT_EQ(rtl::fnv1a((uint8_t)4), 17585875);
  ASSERT_EQ(rtl::fnv1a((uint8_t)5), 808256);
  ASSERT_EQ(rtl::fnv1a((uint8_t)6), 51141113);
  ASSERT_EQ(rtl::fnv1a((uint8_t)7), 34363494);
  ASSERT_EQ(rtl::fnv1a((uint8_t)8), 218917303);
  ASSERT_EQ(rtl::fnv1a((uint8_t)9), 202139684);
  ASSERT_EQ(rtl::fnv1a((uint8_t)10), 252472541);
  ASSERT_EQ(rtl::fnv1a((uint8_t)11), 235694922);
  ASSERT_EQ(rtl::fnv1a((uint8_t)12), 151806827);
  ASSERT_EQ(rtl::fnv1a((uint8_t)13), 135029208);
  ASSERT_EQ(rtl::fnv1a((uint8_t)14), 185362065);
  ASSERT_EQ(rtl::fnv1a((uint8_t)15), 168584446);
  ASSERT_EQ(rtl::fnv1a((uint8_t)16), 353138255);
  ASSERT_EQ(rtl::fnv1a((uint8_t)17), 336360636);
  ASSERT_EQ(rtl::fnv1a((uint8_t)18), 386693493);
  ASSERT_EQ(rtl::fnv1a((uint8_t)19), 369915874);
  ASSERT_EQ(rtl::fnv1a((uint8_t)20), 286027779);
  ASSERT_EQ(rtl::fnv1a((uint8_t)21), 269250160);
  ASSERT_EQ(rtl::fnv1a((uint8_t)22), 319583017);
  ASSERT_EQ(rtl::fnv1a((uint8_t)23), 302805398);
  ASSERT_EQ(rtl::fnv1a((uint8_t)24), 487359207);
  ASSERT_EQ(rtl::fnv1a((uint8_t)25), 470581588);
  ASSERT_EQ(rtl::fnv1a((uint8_t)26), 520914445);
  ASSERT_EQ(rtl::fnv1a((uint8_t)27), 504136826);
  ASSERT_EQ(rtl::fnv1a((uint8_t)28), 420248731);
  ASSERT_EQ(rtl::fnv1a((uint8_t)29), 403471112);
  ASSERT_EQ(rtl::fnv1a((uint8_t)30), 453803969);
  ASSERT_EQ(rtl::fnv1a((uint8_t)31), 437026350);
  ASSERT_EQ(rtl::fnv1a((uint8_t)32), 621580159);
  ASSERT_EQ(rtl::fnv1a((uint8_t)33), 604802540);
  ASSERT_EQ(rtl::fnv1a((uint8_t)34), 655135397);
  ASSERT_EQ(rtl::fnv1a((uint8_t)35), 638357778);
  ASSERT_EQ(rtl::fnv1a((uint8_t)36), 554469683);
  ASSERT_EQ(rtl::fnv1a((uint8_t)37), 537692064);
  ASSERT_EQ(rtl::fnv1a((uint8_t)38), 588024921);
  ASSERT_EQ(rtl::fnv1a((uint8_t)39), 571247302);
  ASSERT_EQ(rtl::fnv1a((uint8_t)40), 755801111);
  ASSERT_EQ(rtl::fnv1a((uint8_t)41), 739023492);
  ASSERT_EQ(rtl::fnv1a((uint8_t)42), 789356349);
  ASSERT_EQ(rtl::fnv1a((uint8_t)43), 772578730);
  ASSERT_EQ(rtl::fnv1a((uint8_t)44), 688690635);
  ASSERT_EQ(rtl::fnv1a((uint8_t)45), 671913016);
  ASSERT_EQ(rtl::fnv1a((uint8_t)46), 722245873);
  ASSERT_EQ(rtl::fnv1a((uint8_t)47), 705468254);
  ASSERT_EQ(rtl::fnv1a((uint8_t)48), 890022063);
  ASSERT_EQ(rtl::fnv1a((uint8_t)49), 873244444);
  ASSERT_EQ(rtl::fnv1a((uint8_t)50), 923577301);
  ASSERT_EQ(rtl::fnv1a((uint8_t)51), 906799682);
  ASSERT_EQ(rtl::fnv1a((uint8_t)52), 822911587);
  ASSERT_EQ(rtl::fnv1a((uint8_t)53), 806133968);
  ASSERT_EQ(rtl::fnv1a((uint8_t)54), 856466825);
  ASSERT_EQ(rtl::fnv1a((uint8_t)55), 839689206);
  ASSERT_EQ(rtl::fnv1a((uint8_t)56), 1024243015);
  ASSERT_EQ(rtl::fnv1a((uint8_t)57), 1007465396);
  ASSERT_EQ(rtl::fnv1a((uint8_t)58), 1057798253);
  ASSERT_EQ(rtl::fnv1a((uint8_t)59), 1041020634);
  ASSERT_EQ(rtl::fnv1a((uint8_t)60), 957132539);
  ASSERT_EQ(rtl::fnv1a((uint8_t)61), 940354920);
  ASSERT_EQ(rtl::fnv1a((uint8_t)62), 990687777);
  ASSERT_EQ(rtl::fnv1a((uint8_t)63), 973910158);
  ASSERT_EQ(rtl::fnv1a((uint8_t)64), 3305896031);
  ASSERT_EQ(rtl::fnv1a((uint8_t)65), 3289118412);
  ASSERT_EQ(rtl::fnv1a((uint8_t)66), 3339451269);
  ASSERT_EQ(rtl::fnv1a((uint8_t)67), 3322673650);
  ASSERT_EQ(rtl::fnv1a((uint8_t)68), 3238785555);
  ASSERT_EQ(rtl::fnv1a((uint8_t)69), 3222007936);
  ASSERT_EQ(rtl::fnv1a((uint8_t)70), 3272340793);
  ASSERT_EQ(rtl::fnv1a((uint8_t)71), 3255563174);
  ASSERT_EQ(rtl::fnv1a((uint8_t)72), 3440116983);
  ASSERT_EQ(rtl::fnv1a((uint8_t)73), 3423339364);
  ASSERT_EQ(rtl::fnv1a((uint8_t)74), 3473672221);
  ASSERT_EQ(rtl::fnv1a((uint8_t)75), 3456894602);
  ASSERT_EQ(rtl::fnv1a((uint8_t)76), 3373006507);
  ASSERT_EQ(rtl::fnv1a((uint8_t)77), 3356228888);
  ASSERT_EQ(rtl::fnv1a((uint8_t)78), 3406561745);
  ASSERT_EQ(rtl::fnv1a((uint8_t)79), 3389784126);
  ASSERT_EQ(rtl::fnv1a((uint8_t)80), 3574337935);
  ASSERT_EQ(rtl::fnv1a((uint8_t)81), 3557560316);
  ASSERT_EQ(rtl::fnv1a((uint8_t)82), 3607893173);
  ASSERT_EQ(rtl::fnv1a((uint8_t)83), 3591115554);
  ASSERT_EQ(rtl::fnv1a((uint8_t)84), 3507227459);
  ASSERT_EQ(rtl::fnv1a((uint8_t)85), 3490449840);
  ASSERT_EQ(rtl::fnv1a((uint8_t)86), 3540782697);
  ASSERT_EQ(rtl::fnv1a((uint8_t)87), 3524005078);
  ASSERT_EQ(rtl::fnv1a((uint8_t)88), 3708558887);
  ASSERT_EQ(rtl::fnv1a((uint8_t)89), 3691781268);
  ASSERT_EQ(rtl::fnv1a((uint8_t)90), 3742114125);
  ASSERT_EQ(rtl::fnv1a((uint8_t)91), 3725336506);
  ASSERT_EQ(rtl::fnv1a((uint8_t)92), 3641448411);
  ASSERT_EQ(rtl::fnv1a((uint8_t)93), 3624670792);
  ASSERT_EQ(rtl::fnv1a((uint8_t)94), 3675003649);
  ASSERT_EQ(rtl::fnv1a((uint8_t)95), 3658226030);
  ASSERT_EQ(rtl::fnv1a((uint8_t)96), 3842779839);
  ASSERT_EQ(rtl::fnv1a((uint8_t)97), 3826002220);
  ASSERT_EQ(rtl::fnv1a((uint8_t)98), 3876335077);
  ASSERT_EQ(rtl::fnv1a((uint8_t)99), 3859557458);
  ASSERT_EQ(rtl::fnv1a((uint8_t)100), 3775669363);
  ASSERT_EQ(rtl::fnv1a((uint8_t)101), 3758891744);
  ASSERT_EQ(rtl::fnv1a((uint8_t)102), 3809224601);
  ASSERT_EQ(rtl::fnv1a((uint8_t)103), 3792446982);
  ASSERT_EQ(rtl::fnv1a((uint8_t)104), 3977000791);
  ASSERT_EQ(rtl::fnv1a((uint8_t)105), 3960223172);
  ASSERT_EQ(rtl::fnv1a((uint8_t)106), 4010556029);
  ASSERT_EQ(rtl::fnv1a((uint8_t)107), 3993778410);
  ASSERT_EQ(rtl::fnv1a((uint8_t)108), 3909890315);
  ASSERT_EQ(rtl::fnv1a((uint8_t)109), 3893112696);
  ASSERT_EQ(rtl::fnv1a((uint8_t)110), 3943445553);
  ASSERT_EQ(rtl::fnv1a((uint8_t)111), 3926667934);
  ASSERT_EQ(rtl::fnv1a((uint8_t)112), 4111221743);
  ASSERT_EQ(rtl::fnv1a((uint8_t)113), 4094444124);
  ASSERT_EQ(rtl::fnv1a((uint8_t)114), 4144776981);
  ASSERT_EQ(rtl::fnv1a((uint8_t)115), 4127999362);
  ASSERT_EQ(rtl::fnv1a((uint8_t)116), 4044111267);
  ASSERT_EQ(rtl::fnv1a((uint8_t)117), 4027333648);
  ASSERT_EQ(rtl::fnv1a((uint8_t)118), 4077666505);
  ASSERT_EQ(rtl::fnv1a((uint8_t)119), 4060888886);
  ASSERT_EQ(rtl::fnv1a((uint8_t)120), 4245442695);
  ASSERT_EQ(rtl::fnv1a((uint8_t)121), 4228665076);
  ASSERT_EQ(rtl::fnv1a((uint8_t)122), 4278997933);
  ASSERT_EQ(rtl::fnv1a((uint8_t)123), 4262220314);
  ASSERT_EQ(rtl::fnv1a((uint8_t)124), 4178332219);
  ASSERT_EQ(rtl::fnv1a((uint8_t)125), 4161554600);
  ASSERT_EQ(rtl::fnv1a((uint8_t)126), 4211887457);
  ASSERT_EQ(rtl::fnv1a((uint8_t)127), 4195109838);
  ASSERT_EQ(rtl::fnv1a((uint8_t)128), 2232128415);
  ASSERT_EQ(rtl::fnv1a((uint8_t)129), 2215350796);
  ASSERT_EQ(rtl::fnv1a((uint8_t)130), 2265683653);
  ASSERT_EQ(rtl::fnv1a((uint8_t)131), 2248906034);
  ASSERT_EQ(rtl::fnv1a((uint8_t)132), 2165017939);
  ASSERT_EQ(rtl::fnv1a((uint8_t)133), 2148240320);
  ASSERT_EQ(rtl::fnv1a((uint8_t)134), 2198573177);
  ASSERT_EQ(rtl::fnv1a((uint8_t)135), 2181795558);
  ASSERT_EQ(rtl::fnv1a((uint8_t)136), 2366349367);
  ASSERT_EQ(rtl::fnv1a((uint8_t)137), 2349571748);
  ASSERT_EQ(rtl::fnv1a((uint8_t)138), 2399904605);
  ASSERT_EQ(rtl::fnv1a((uint8_t)139), 2383126986);
  ASSERT_EQ(rtl::fnv1a((uint8_t)140), 2299238891);
  ASSERT_EQ(rtl::fnv1a((uint8_t)141), 2282461272);
  ASSERT_EQ(rtl::fnv1a((uint8_t)142), 2332794129);
  ASSERT_EQ(rtl::fnv1a((uint8_t)143), 2316016510);
  ASSERT_EQ(rtl::fnv1a((uint8_t)144), 2500570319);
  ASSERT_EQ(rtl::fnv1a((uint8_t)145), 2483792700);
  ASSERT_EQ(rtl::fnv1a((uint8_t)146), 2534125557);
  ASSERT_EQ(rtl::fnv1a((uint8_t)147), 2517347938);
  ASSERT_EQ(rtl::fnv1a((uint8_t)148), 2433459843);
  ASSERT_EQ(rtl::fnv1a((uint8_t)149), 2416682224);
  ASSERT_EQ(rtl::fnv1a((uint8_t)150), 2467015081);
  ASSERT_EQ(rtl::fnv1a((uint8_t)151), 2450237462);
  ASSERT_EQ(rtl::fnv1a((uint8_t)152), 2634791271);
  ASSERT_EQ(rtl::fnv1a((uint8_t)153), 2618013652);
  ASSERT_EQ(rtl::fnv1a((uint8_t)154), 2668346509);
  ASSERT_EQ(rtl::fnv1a((uint8_t)155), 2651568890);
  ASSERT_EQ(rtl::fnv1a((uint8_t)156), 2567680795);
  ASSERT_EQ(rtl::fnv1a((uint8_t)157), 2550903176);
  ASSERT_EQ(rtl::fnv1a((uint8_t)158), 2601236033);
  ASSERT_EQ(rtl::fnv1a((uint8_t)159), 2584458414);
  ASSERT_EQ(rtl::fnv1a((uint8_t)160), 2769012223);
  ASSERT_EQ(rtl::fnv1a((uint8_t)161), 2752234604);
  ASSERT_EQ(rtl::fnv1a((uint8_t)162), 2802567461);
  ASSERT_EQ(rtl::fnv1a((uint8_t)163), 2785789842);
  ASSERT_EQ(rtl::fnv1a((uint8_t)164), 2701901747);
  ASSERT_EQ(rtl::fnv1a((uint8_t)165), 2685124128);
  ASSERT_EQ(rtl::fnv1a((uint8_t)166), 2735456985);
  ASSERT_EQ(rtl::fnv1a((uint8_t)167), 2718679366);
  ASSERT_EQ(rtl::fnv1a((uint8_t)168), 2903233175);
  ASSERT_EQ(rtl::fnv1a((uint8_t)169), 2886455556);
  ASSERT_EQ(rtl::fnv1a((uint8_t)170), 2936788413);
  ASSERT_EQ(rtl::fnv1a((uint8_t)171), 2920010794);
  ASSERT_EQ(rtl::fnv1a((uint8_t)172), 2836122699);
  ASSERT_EQ(rtl::fnv1a((uint8_t)173), 2819345080);
  ASSERT_EQ(rtl::fnv1a((uint8_t)174), 2869677937);
  ASSERT_EQ(rtl::fnv1a((uint8_t)175), 2852900318);
  ASSERT_EQ(rtl::fnv1a((uint8_t)176), 3037454127);
  ASSERT_EQ(rtl::fnv1a((uint8_t)177), 3020676508);
  ASSERT_EQ(rtl::fnv1a((uint8_t)178), 3071009365);
  ASSERT_EQ(rtl::fnv1a((uint8_t)179), 3054231746);
  ASSERT_EQ(rtl::fnv1a((uint8_t)180), 2970343651);
  ASSERT_EQ(rtl::fnv1a((uint8_t)181), 2953566032);
  ASSERT_EQ(rtl::fnv1a((uint8_t)182), 3003898889);
  ASSERT_EQ(rtl::fnv1a((uint8_t)183), 2987121270);
  ASSERT_EQ(rtl::fnv1a((uint8_t)184), 3171675079);
  ASSERT_EQ(rtl::fnv1a((uint8_t)185), 3154897460);
  ASSERT_EQ(rtl::fnv1a((uint8_t)186), 3205230317);
  ASSERT_EQ(rtl::fnv1a((uint8_t)187), 3188452698);
  ASSERT_EQ(rtl::fnv1a((uint8_t)188), 3104564603);
  ASSERT_EQ(rtl::fnv1a((uint8_t)189), 3087786984);
  ASSERT_EQ(rtl::fnv1a((uint8_t)190), 3138119841);
  ASSERT_EQ(rtl::fnv1a((uint8_t)191), 3121342222);
  ASSERT_EQ(rtl::fnv1a((uint8_t)192), 1158360799);
  ASSERT_EQ(rtl::fnv1a((uint8_t)193), 1141583180);
  ASSERT_EQ(rtl::fnv1a((uint8_t)194), 1191916037);
  ASSERT_EQ(rtl::fnv1a((uint8_t)195), 1175138418);
  ASSERT_EQ(rtl::fnv1a((uint8_t)196), 1091250323);
  ASSERT_EQ(rtl::fnv1a((uint8_t)197), 1074472704);
  ASSERT_EQ(rtl::fnv1a((uint8_t)198), 1124805561);
  ASSERT_EQ(rtl::fnv1a((uint8_t)199), 1108027942);
  ASSERT_EQ(rtl::fnv1a((uint8_t)200), 1292581751);
  ASSERT_EQ(rtl::fnv1a((uint8_t)201), 1275804132);
  ASSERT_EQ(rtl::fnv1a((uint8_t)202), 1326136989);
  ASSERT_EQ(rtl::fnv1a((uint8_t)203), 1309359370);
  ASSERT_EQ(rtl::fnv1a((uint8_t)204), 1225471275);
  ASSERT_EQ(rtl::fnv1a((uint8_t)205), 1208693656);
  ASSERT_EQ(rtl::fnv1a((uint8_t)206), 1259026513);
  ASSERT_EQ(rtl::fnv1a((uint8_t)207), 1242248894);
  ASSERT_EQ(rtl::fnv1a((uint8_t)208), 1426802703);
  ASSERT_EQ(rtl::fnv1a((uint8_t)209), 1410025084);
  ASSERT_EQ(rtl::fnv1a((uint8_t)210), 1460357941);
  ASSERT_EQ(rtl::fnv1a((uint8_t)211), 1443580322);
  ASSERT_EQ(rtl::fnv1a((uint8_t)212), 1359692227);
  ASSERT_EQ(rtl::fnv1a((uint8_t)213), 1342914608);
  ASSERT_EQ(rtl::fnv1a((uint8_t)214), 1393247465);
  ASSERT_EQ(rtl::fnv1a((uint8_t)215), 1376469846);
  ASSERT_EQ(rtl::fnv1a((uint8_t)216), 1561023655);
  ASSERT_EQ(rtl::fnv1a((uint8_t)217), 1544246036);
  ASSERT_EQ(rtl::fnv1a((uint8_t)218), 1594578893);
  ASSERT_EQ(rtl::fnv1a((uint8_t)219), 1577801274);
  ASSERT_EQ(rtl::hash<uint8_t>{}(219), 1577801274);
  ASSERT_EQ(rtl::fnv1a((uint8_t)220), 1493913179);
  ASSERT_EQ(rtl::fnv1a((uint8_t)221), 1477135560);
  ASSERT_EQ(rtl::fnv1a((uint8_t)222), 1527468417);
  ASSERT_EQ(rtl::fnv1a((uint8_t)223), 1510690798);
  ASSERT_EQ(rtl::fnv1a((uint8_t)224), 1695244607);
  ASSERT_EQ(rtl::fnv1a((uint8_t)225), 1678466988);
  ASSERT_EQ(rtl::fnv1a((uint8_t)226), 1728799845);
  ASSERT_EQ(rtl::fnv1a((uint8_t)227), 1712022226);
  ASSERT_EQ(rtl::fnv1a((uint8_t)228), 1628134131);
  ASSERT_EQ(rtl::fnv1a((uint8_t)229), 1611356512);
  ASSERT_EQ(rtl::fnv1a((uint8_t)230), 1661689369);
  ASSERT_EQ(rtl::fnv1a((uint8_t)231), 1644911750);
  ASSERT_EQ(rtl::fnv1a((uint8_t)232), 1829465559);
  ASSERT_EQ(rtl::fnv1a((uint8_t)233), 1812687940);
  ASSERT_EQ(rtl::fnv1a((uint8_t)234), 1863020797);
  ASSERT_EQ(rtl::fnv1a((uint8_t)235), 1846243178);
  ASSERT_EQ(rtl::fnv1a((uint8_t)236), 1762355083);
  ASSERT_EQ(rtl::fnv1a((uint8_t)237), 1745577464);
  ASSERT_EQ(rtl::fnv1a((uint8_t)238), 1795910321);
  ASSERT_EQ(rtl::fnv1a((uint8_t)239), 1779132702);
  ASSERT_EQ(rtl::fnv1a((uint8_t)240), 1963686511);
  ASSERT_EQ(rtl::fnv1a((uint8_t)241), 1946908892);
  ASSERT_EQ(rtl::fnv1a((uint8_t)242), 1997241749);
  ASSERT_EQ(rtl::fnv1a((uint8_t)243), 1980464130);
  ASSERT_EQ(rtl::fnv1a((uint8_t)244), 1896576035);
  ASSERT_EQ(rtl::fnv1a((uint8_t)245), 1879798416);
  ASSERT_EQ(rtl::fnv1a((uint8_t)246), 1930131273);
  ASSERT_EQ(rtl::fnv1a((uint8_t)247), 1913353654);
  ASSERT_EQ(rtl::fnv1a((uint8_t)248), 2097907463);
  ASSERT_EQ(rtl::fnv1a((uint8_t)249), 2081129844);
  ASSERT_EQ(rtl::fnv1a((uint8_t)250), 2131462701);
  ASSERT_EQ(rtl::fnv1a((uint8_t)251), 2114685082);
  ASSERT_EQ(rtl::fnv1a((uint8_t)252), 2030796987);
  ASSERT_EQ(rtl::fnv1a((uint8_t)253), 2014019368);
  ASSERT_EQ(rtl::fnv1a((uint8_t)254), 2064352225);
  ASSERT_EQ(rtl::fnv1a((uint8_t)255), 2047574606);
}
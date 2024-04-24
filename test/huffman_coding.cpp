#include "huffman_coding.h"

#include <gtest/gtest.h>

#include <sstream>


constexpr char sample_text[] = R""""(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vitae risus non sem posuere aliquam. Nulla nisi magna, interdum nec pellentesque sed, sodales et arcu. Morbi lobortis mi sit amet odio malesuada imperdiet. Nunc a ipsum sollicitudin, luctus dolor non, ultricies magna. Mauris pretium ligula vel vehicula posuere. Suspendisse congue venenatis ex ut vestibulum. In vel nulla lacinia nisl aliquam venenatis. Quisque id tristique lorem. Donec eu massa luctus, lobortis dolor sed, ultrices dui. Morbi eu ligula ac lacus blandit mattis eget nec diam. Phasellus suscipit augue in mollis dignissim. Integer sit amet elit nec turpis consequat tristique vitae ut diam. Vivamus ac varius nisl.

Aliquam facilisis magna tortor, finibus vehicula mauris bibendum a. Nulla condimentum interdum tortor eu finibus. Curabitur in placerat lacus. Aenean orci lacus, bibendum sed dui sed, efficitur lacinia orci. Praesent fringilla risus at mi auctor mattis. Phasellus facilisis ante quis dolor faucibus gravida. Vestibulum aliquet lorem neque, sit amet suscipit ipsum congue ac. Quisque aliquet nisl eros. Vestibulum magna lectus, ultrices non tempor sit amet, pulvinar quis sem. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Suspendisse sodales velit sit amet eleifend luctus.

Mauris ornare, tellus a tempus semper, ex felis volutpat magna, at ultricies arcu massa in magna. Proin nisl lacus, ullamcorper a vestibulum in, posuere in elit. Curabitur tincidunt hendrerit erat, id aliquam nibh sodales nec. Suspendisse in rutrum quam. Nunc quis dui sit amet nisi dictum aliquet eu a turpis. Curabitur auctor maximus lectus quis ullamcorper. Ut consequat massa sit amet sapien convallis congue. Etiam feugiat iaculis magna interdum faucibus. Vivamus et eleifend metus. Proin in nisi rhoncus, interdum urna vitae, suscipit augue. Pellentesque mattis molestie lectus non pharetra. Nulla facilisi. Curabitur nulla leo, mollis eget purus vitae, luctus accumsan neque.

Quisque rhoncus tempus ligula, a laoreet urna varius ac. Donec non pharetra risus. Pellentesque maximus maximus finibus. Aenean lorem nulla, iaculis quis fringilla a, pellentesque vel arcu. Integer venenatis eros ex, a dignissim ligula tempor ut. Quisque sollicitudin, risus sit amet cursus sollicitudin, orci lorem facilisis elit, at pharetra tellus orci in justo. Proin sodales semper lacus et sagittis. Vestibulum aliquam varius elit consectetur congue. Nunc aliquam convallis dolor ut vehicula. Aliquam erat volutpat. Mauris eu tortor eu est elementum mattis. Sed vestibulum placerat ex eget ultrices. Maecenas tempus lacus felis, vel sagittis erat tincidunt vitae. Aliquam condimentum ut mi molestie laoreet. Duis id suscipit massa. Phasellus lectus quam, tristique a orci vel, pharetra aliquet sem.

Curabitur porttitor sapien turpis, sit amet aliquet dolor efficitur eu. Sed nulla nisi, suscipit ut gravida ut, iaculis non elit. Donec purus lorem, aliquet nec sollicitudin ullamcorper, varius sit amet massa. Nulla ullamcorper nibh lobortis odio congue tristique. Sed nibh magna, rhoncus eu mauris quis, aliquam gravida purus. Fusce sit amet ante viverra nisl interdum tempus. Duis justo elit, porta eu consectetur id, ullamcorper quis lectus. Mauris nisi tortor, aliquam eget est id, luctus ultrices enim. Quisque id neque nulla. Nulla facilisi. Maecenas ligula nunc, commodo at tristique sed, venenatis sit amet magna. Morbi tincidunt porttitor nibh ut feugiat. )"""";


TEST(HuffmanCoding, EncodeDecode)
{
    auto huffman_tree = build_huffman_tree(sample_text, sample_text + sizeof(sample_text) / sizeof(sample_text[0]));

    std::ostringstream oss {std::ios_base::binary};
    HuffmanStringEncoder encoder {oss, huffman_tree.get()};
    encoder << sample_text;
    encoder.finalize();

    std::istringstream iss {oss.str(), std::ios_base::binary};
    HuffmanStringDecoder decoder {iss, *huffman_tree};
    std::string decoded_sample_text;
    decoder >> decoded_sample_text;

    EXPECT_EQ(sample_text, decoded_sample_text);
}

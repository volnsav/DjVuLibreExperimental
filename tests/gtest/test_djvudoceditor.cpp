#include <gtest/gtest.h>

#include "DjVuDocEditor.h"

TEST(DjVuDocEditorTest, CreateEmptyEditorInitializes)
{
  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait();
  ASSERT_TRUE(editor != 0);

  EXPECT_TRUE(editor->is_init_complete());
  EXPECT_TRUE(editor->is_init_ok());
  EXPECT_TRUE(editor->inherits("DjVuDocEditor"));
  EXPECT_TRUE(editor->inherits("DjVuDocument"));
  EXPECT_TRUE(editor->inherits("DjVuPort"));
}

TEST(DjVuDocEditorTest, EmptyEditorSaveCapabilitiesMatchUnknownOriginalType)
{
  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait();
  ASSERT_TRUE(editor != 0);

  EXPECT_EQ(DjVuDocument::UNKNOWN_TYPE, editor->get_orig_doc_type());
  EXPECT_FALSE(editor->can_be_saved());
  EXPECT_EQ(DjVuDocument::UNKNOWN_TYPE, editor->get_save_doc_type());
}

TEST(DjVuDocEditorTest, DocUrlIsPresent)
{
  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait();
  ASSERT_TRUE(editor != 0);

  GURL url = editor->get_doc_url();
  EXPECT_FALSE(url.is_empty());
}

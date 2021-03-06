// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/table/ng_table_layout_algorithm_helpers.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class NGTableAlgorithmHelpersTest : public testing::Test {
  void SetUp() override {}

 public:
  NGTableTypes::Column MakeColumn(int min_width,
                                  int max_width,
                                  base::Optional<float> percent = base::nullopt,
                                  bool is_constrained = false) {
    return NGTableTypes::Column{LayoutUnit(min_width), LayoutUnit(max_width),
                                percent, is_constrained};
  }

  NGTableTypes::Row MakeRow(int block_size,
                            bool is_constrained = false,
                            bool has_rowspan_start = false,
                            base::Optional<float> percent = base::nullopt) {
    return NGTableTypes::Row{
        LayoutUnit(block_size), LayoutUnit(), percent,           0,    0,
        is_constrained,         false,        has_rowspan_start, false};
  }

  NGTableTypes::Section MakeSection(
      NGTableTypes::Rows* rows,
      int block_size,
      wtf_size_t rowspan = 1,
      base::Optional<float> percent = base::nullopt) {
    wtf_size_t start_row = rows->size();
    for (wtf_size_t i = 0; i < rowspan; i++)
      rows->push_back(MakeRow(10));
    bool is_constrained = percent || block_size != 0;
    bool is_tbody = true;
    return NGTableTypes::Section{
        start_row, rowspan, LayoutUnit(block_size), percent, is_constrained,
        is_tbody,  false};
  }
};

TEST_F(NGTableAlgorithmHelpersTest, DistributeColspanAutoPercent) {
  NGTableTypes::ColspanCell colspan_cell(NGTableTypes::CellInlineConstraint(),
                                         0, 3);
  colspan_cell.start_column = 0;
  colspan_cell.span = 3;

  NGTableTypes::Columns column_constraints;

  colspan_cell.cell_inline_constraint.percent = 60.0f;

  // Distribute over non-percent columns proportial to max size.
  // Columns: 10px, 20px, 30%
  // Distribute 60%: 10% 20% 30%
  column_constraints.Shrink(0);
  column_constraints.push_back(MakeColumn(0, 10));
  column_constraints.push_back(MakeColumn(0, 20));
  column_constraints.push_back(MakeColumn(0, 10, 30));
  NGTableAlgorithmHelpers::DistributeColspanCellToColumns(
      colspan_cell, LayoutUnit(), false, &column_constraints);
  EXPECT_EQ(*column_constraints[0].percent, 10);
  EXPECT_EQ(*column_constraints[1].percent, 20);

  // Distribute evenly over empty columns.
  // Columns: 0px 0px 10%
  // Distribute 60%: 25% 25% 10%
  column_constraints.Shrink(0);
  column_constraints.push_back(MakeColumn(0, 0));
  column_constraints.push_back(MakeColumn(0, 0));
  column_constraints.push_back(MakeColumn(0, 10, 10));
  NGTableAlgorithmHelpers::DistributeColspanCellToColumns(
      colspan_cell, LayoutUnit(), false, &column_constraints);
  EXPECT_EQ(*column_constraints[0].percent, 25);
  EXPECT_EQ(*column_constraints[1].percent, 25);
}

TEST_F(NGTableAlgorithmHelpersTest, DistributeColspanAutoSizeUnconstrained) {
  NGTableTypes::ColspanCell colspan_cell(NGTableTypes::CellInlineConstraint(),
                                         0, 3);
  colspan_cell.start_column = 0;
  colspan_cell.span = 3;

  NGTableTypes::Columns column_constraints;

  // Columns distributing over auto columns.
  colspan_cell.cell_inline_constraint.min_inline_size = LayoutUnit(100);
  colspan_cell.cell_inline_constraint.max_inline_size = LayoutUnit(100);
  // Distribute over non-percent columns proportial to max size.
  // Columns min/max: 0/10, 0/10, 0/20
  // Distribute 25, 25, 50
  column_constraints.Shrink(0);
  column_constraints.push_back(MakeColumn(0, 10));
  column_constraints.push_back(MakeColumn(0, 10));
  column_constraints.push_back(MakeColumn(0, 20));
  NGTableAlgorithmHelpers::DistributeColspanCellToColumns(
      colspan_cell, LayoutUnit(), false, &column_constraints);
  EXPECT_EQ(*column_constraints[0].min_inline_size, 25);
  EXPECT_EQ(*column_constraints[1].min_inline_size, 25);
  EXPECT_EQ(*column_constraints[2].min_inline_size, 50);
}

TEST_F(NGTableAlgorithmHelpersTest, DistributeColspanAutoSizeConstrained) {
  NGTableTypes::ColspanCell colspan_cell(NGTableTypes::CellInlineConstraint(),
                                         0, 3);
  colspan_cell.start_column = 0;
  colspan_cell.span = 3;

  NGTableTypes::Columns column_constraints;

  // Columns distributing over auto columns.
  colspan_cell.cell_inline_constraint.min_inline_size = LayoutUnit(100);
  colspan_cell.cell_inline_constraint.max_inline_size = LayoutUnit(100);
  // Distribute over fixed columns proportial to:
  // Columns min/max: 0/10, 0/10, 0/20
  // Distribute 25, 25, 50
  column_constraints.Shrink(0);
  column_constraints.push_back(MakeColumn(0, 10, base::nullopt, true));
  column_constraints.push_back(MakeColumn(10, 10, base::nullopt, true));
  column_constraints.push_back(MakeColumn(0, 20, base::nullopt, true));
  NGTableAlgorithmHelpers::DistributeColspanCellToColumns(
      colspan_cell, LayoutUnit(), false, &column_constraints);
  EXPECT_EQ(*column_constraints[0].min_inline_size, 25);
  EXPECT_EQ(*column_constraints[1].min_inline_size, 25);
  EXPECT_EQ(*column_constraints[2].min_inline_size, 50);
}

TEST_F(NGTableAlgorithmHelpersTest, DistributeColspanAutoExactMaxSize) {
  // If column widths sum match table widths exactly, column widths
  // should not be redistributed at all.
  // The error occurs if widths are redistributed, and column widths
  // change due to floating point rounding.
  LayoutUnit column_widths[] = {LayoutUnit(0.1), LayoutUnit(22.123456),
                                LayoutUnit(33.789012), LayoutUnit(2000.345678)};
  NGTableTypes::Columns column_constraints;
  column_constraints.Shrink(0);
  column_constraints.push_back(NGTableTypes::Column{
      LayoutUnit(0), column_widths[0], base::nullopt, false});
  column_constraints.push_back(NGTableTypes::Column{
      LayoutUnit(3.33333), column_widths[1], base::nullopt, false});
  column_constraints.push_back(NGTableTypes::Column{
      LayoutUnit(3.33333), column_widths[2], base::nullopt, false});
  column_constraints.push_back(NGTableTypes::Column{
      LayoutUnit(0), column_widths[3], base::nullopt, false});

  LayoutUnit assignable_table_inline_size =
      column_widths[0] + column_widths[1] + column_widths[2] + column_widths[3];
  NGTableAlgorithmHelpers::SynchronizeAssignableTableInlineSizeAndColumns(
      assignable_table_inline_size, LayoutUnit(), false, &column_constraints);
  EXPECT_EQ(column_constraints[0].computed_inline_size, column_widths[0]);
  EXPECT_EQ(column_constraints[1].computed_inline_size, column_widths[1]);
  EXPECT_EQ(column_constraints[2].computed_inline_size, column_widths[2]);
  EXPECT_EQ(column_constraints[3].computed_inline_size, column_widths[3]);
}

TEST_F(NGTableAlgorithmHelpersTest, ComputeGridInlineMinmax) {
  NGTableTypes::Columns column_constraints;

  LayoutUnit undistributable_space;
  bool is_fixed_layout = false;
  bool containing_block_expects_minmax_without_percentages = false;

  // No percentages, just sums up min/max.
  column_constraints.push_back(MakeColumn(10, 100));
  column_constraints.push_back(MakeColumn(20, 200));
  column_constraints.push_back(MakeColumn(30, 300));

  MinMaxSizes minmax;
  NGTableAlgorithmHelpers::ComputeGridInlineMinmax(
      column_constraints, is_fixed_layout,
      containing_block_expects_minmax_without_percentages,
      undistributable_space, &minmax);
  EXPECT_EQ(minmax.min_size, LayoutUnit(60));
  EXPECT_EQ(minmax.max_size, LayoutUnit(600));

  // Percentage: 99px max size/10% cell =>
  // table max size of 100%/10% * 99px
  minmax = MinMaxSizes();
  column_constraints.Shrink(0);
  column_constraints.push_back(MakeColumn(10, 99, 10));
  column_constraints.push_back(MakeColumn(10, 10));
  column_constraints.push_back(MakeColumn(10, 10));
  NGTableAlgorithmHelpers::ComputeGridInlineMinmax(
      column_constraints, is_fixed_layout,
      containing_block_expects_minmax_without_percentages,
      undistributable_space, &minmax);
  EXPECT_EQ(minmax.min_size, LayoutUnit(30));
  EXPECT_EQ(minmax.max_size, LayoutUnit(990));

  // Without percent, minmax ignores percent
  minmax = MinMaxSizes();
  containing_block_expects_minmax_without_percentages = true;
  NGTableAlgorithmHelpers::ComputeGridInlineMinmax(
      column_constraints, is_fixed_layout,
      containing_block_expects_minmax_without_percentages,
      undistributable_space, &minmax);
  EXPECT_EQ(minmax.min_size, LayoutUnit(30));
  EXPECT_EQ(minmax.max_size, LayoutUnit(119));

  // Percentage: total percentage of 20%, and non-percent width of 800 =>
  // table max size of 800 + (20% * 800/80%) = 1000
  minmax = MinMaxSizes();
  containing_block_expects_minmax_without_percentages = false;
  column_constraints.Shrink(0);
  column_constraints.push_back(MakeColumn(10, 100, 10));
  column_constraints.push_back(MakeColumn(10, 10, 10));
  column_constraints.push_back(MakeColumn(10, 800));
  NGTableAlgorithmHelpers::ComputeGridInlineMinmax(
      column_constraints, is_fixed_layout,
      containing_block_expects_minmax_without_percentages,
      undistributable_space, &minmax);
  EXPECT_EQ(minmax.min_size, LayoutUnit(30));
  EXPECT_EQ(minmax.max_size, LayoutUnit(1000));
}

TEST_F(NGTableAlgorithmHelpersTest, DistributeRowspanCellToRows) {
  NGTableTypes::CellBlockConstraint cell_block_constraint{
      LayoutUnit(300),      LayoutUnit(), NGBoxStrut(), 0, 0, 3,
      EVerticalAlign::kTop, true};
  NGTableTypes::RowspanCell rowspan_cell = NGTableTypes::CreateRowspanCell(
      0, 3, &cell_block_constraint, base::nullopt);
  NGTableTypes::Rows rows;

  // Distribute to regular rows, rows grow in proportion to size.
  rows.push_back(MakeRow(10));
  rows.push_back(MakeRow(20));
  rows.push_back(MakeRow(30));
  NGTableAlgorithmHelpers::DistributeRowspanCellToRows(rowspan_cell,
                                                       LayoutUnit(), &rows);
  EXPECT_EQ(rows[0].block_size, LayoutUnit(50));
  EXPECT_EQ(rows[1].block_size, LayoutUnit(100));
  EXPECT_EQ(rows[2].block_size, LayoutUnit(150));

  // If some rows are empty, non-empty row gets everything
  rows.Shrink(0);
  rows.push_back(MakeRow(0));
  rows.push_back(MakeRow(10));
  rows.push_back(MakeRow(0));
  NGTableAlgorithmHelpers::DistributeRowspanCellToRows(rowspan_cell,
                                                       LayoutUnit(), &rows);
  EXPECT_EQ(rows[0].block_size, LayoutUnit(0));
  EXPECT_EQ(rows[1].block_size, LayoutUnit(300));
  EXPECT_EQ(rows[2].block_size, LayoutUnit(0));

  // If all rows are empty,last row gets everything.
  rows.Shrink(0);
  rows.push_back(MakeRow(0));
  rows.push_back(MakeRow(0));
  rows.push_back(MakeRow(0));
  NGTableAlgorithmHelpers::DistributeRowspanCellToRows(rowspan_cell,
                                                       LayoutUnit(), &rows);
  EXPECT_EQ(rows[0].block_size, LayoutUnit(0));
  EXPECT_EQ(rows[1].block_size, LayoutUnit(0));
  EXPECT_EQ(rows[2].block_size, LayoutUnit(300));
}

TEST_F(NGTableAlgorithmHelpersTest, DistributeSectionFixedBlockSizeToRows) {
  NGTableTypes::Rows rows;

  // Percentage rows get percentage, rest is distributed evenly.
  rows.push_back(MakeRow(100));
  rows.push_back(MakeRow(100, true, false, 50));
  rows.push_back(MakeRow(100));
  NGTableAlgorithmHelpers::DistributeSectionFixedBlockSizeToRows(
      0, 3, LayoutUnit(1000), LayoutUnit(), LayoutUnit(1000), &rows);
  EXPECT_EQ(rows[0].block_size, LayoutUnit(250));
  EXPECT_EQ(rows[1].block_size, LayoutUnit(500));
  EXPECT_EQ(rows[2].block_size, LayoutUnit(250));
}

TEST_F(NGTableAlgorithmHelpersTest, DistributeTableBlockSizeToSections) {
  NGTableTypes::Sections sections;
  NGTableTypes::Rows rows;

  // Empty sections only grow if there are no other growable sections.
  sections.push_back(MakeSection(&rows, 0));
  sections.push_back(MakeSection(&rows, 100));
  NGTableAlgorithmHelpers::DistributeTableBlockSizeToSections(
      LayoutUnit(), LayoutUnit(500), &sections, &rows);
  EXPECT_EQ(sections[0].block_size, LayoutUnit(0));

  // Sections with % block size grow to percentage.
  sections.Shrink(0);
  rows.Shrink(0);
  sections.push_back(MakeSection(&rows, 100, 1, 90));
  sections.push_back(MakeSection(&rows, 100));
  NGTableAlgorithmHelpers::DistributeTableBlockSizeToSections(
      LayoutUnit(), LayoutUnit(1000), &sections, &rows);
  EXPECT_EQ(sections[0].block_size, LayoutUnit(900));
  EXPECT_EQ(sections[1].block_size, LayoutUnit(100));

  // When table height is greater than sum of intrinsic heights,
  // intrinsic heights are computed, and then they grow in
  // proportion to intrinsic height.
  sections.Shrink(0);
  rows.Shrink(0);
  sections.push_back(MakeSection(&rows, 100, 1, 30));
  sections.push_back(MakeSection(&rows, 100));
  // 30% section grows to 300px.
  // Extra 600 px is distributed between 300 and 100 px proportionally.
  // TODO(atotic) Is this what we want? FF/Edge/Legacy all disagree.
  NGTableAlgorithmHelpers::DistributeTableBlockSizeToSections(
      LayoutUnit(), LayoutUnit(1000), &sections, &rows);
  EXPECT_EQ(sections[0].block_size, LayoutUnit(750));
  EXPECT_EQ(sections[1].block_size, LayoutUnit(250));

  // If there is a constrained section, and an unconstrained section,
  // unconstrained section grows.
  sections.Shrink(0);
  rows.Shrink(0);
  NGTableTypes::Section section(MakeSection(&rows, 100));
  section.is_constrained = false;
  sections.push_back(section);
  sections.push_back(MakeSection(&rows, 100));
  NGTableAlgorithmHelpers::DistributeTableBlockSizeToSections(
      LayoutUnit(), LayoutUnit(1000), &sections, &rows);
  EXPECT_EQ(sections[0].block_size, LayoutUnit(900));
  EXPECT_EQ(sections[1].block_size, LayoutUnit(100));
}

}  // namespace blink

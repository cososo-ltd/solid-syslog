"""Regression tests for the report dressing hook (hooks/report_chrome.py).

Run:  python3 hooks/test_report_chrome.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')

The strict build and the offline bundle check prove that every link the hook
writes resolves. What they cannot prove is that the right pieces were lifted
and the wrong ones dropped: a search box that cannot work, a credit that names
the wrong tool, a title line that does not lead back to the page that
introduces the report.
"""

import os
import sys
import tempfile
import textwrap
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import report_chrome as h  # noqa: E402

TEMPLATE = textwrap.dedent(
    """\
    <html><head>
    <link rel="stylesheet" href="../../assets/stylesheets/main.css">
    <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Open+Sans">
    </head>
    <body dir="ltr" data-md-color-scheme="default" data-md-color-primary="white">
    <header class="md-header" data-md-component="header">
      <a href="https://www.cososo.co.uk" class="md-logo"><img src="../../assets/images/cososo-logo.svg" alt="logo"></a>
      <label class="md-header__button md-icon" for="__drawer"><svg></svg></label>
      <div class="md-header__title" data-md-component="header-title">
      <div class="md-header__topic"><span class="md-ellipsis">SolidSyslog</span></div>
      <div class="md-header__topic" data-md-component="header-topic">
        <span class="md-ellipsis">
          Coverage
        </span>
      </div>
      </div>
      <form class="md-header__option" data-md-component="palette"><input></form>
      <script>var palette=1;</script>
      <label class="md-header__button md-icon" for="__search"><svg></svg></label>
      <div class="md-search" data-md-component="search"><div class="md-search__inner"><div>x</div></div></div>
      <div class="md-header__source"><a href="https://github.com/cososo-ltd/solid-syslog">repo</a></div>
    </header>
    <nav class="md-tabs"><ul><li><a href="../.." class="md-tabs__link">Home</a></li><li><a href="./" class="md-tabs__link">Testing</a></li></ul></nav>
    <main>Body</main>
    <footer class="md-footer"><img src="../../assets/images/solidsyslog-mark-white.svg">
      <div class="ss-footer-build">Documentation built from main at 1234567, after release 0.3.0.</div>
      Made with
      <a href="https://squidfunk.github.io/mkdocs-material/" target="_blank" rel="noopener">
        Material for MkDocs
      </a>
    </footer>
    <script src="../../assets/javascripts/bundle.79ae519e.min.js"></script>
    <script src="../../javascripts/logo-scroll.js"></script>
    </body></html>
    """
)

REPORT_PAGE = textwrap.dedent(
    """\
    <html><head>
    <link rel="stylesheet" type="text/css" href="../../../gcov.css">
    <!-- site-styles -->
    <link rel="stylesheet" type="text/css" href="../../../solidsyslog.css">
    </head>
    <body>
    <!-- site-header -->
    <header class="ss-header">simple</header>
    <!-- /site-header -->
    <table>coverage</table>
    <!-- site-footer -->
    <footer class="ss-footer">simple</footer>
    <!-- /site-footer -->
    </body></html>
    """
)

CREDIT = 'Reported by <a href="https://example.test/lcov">LCOV</a>'


class Relocation(unittest.TestCase):
    def test_a_site_relative_asset_is_rebased_to_the_deeper_page(self):
        out = h.relocate('src="../../assets/images/cososo-logo.svg"', "testing/coverage/", "testing/coverage-report/Platform/Posix/Source/")
        self.assertEqual(out, 'src="../../../../../assets/images/cososo-logo.svg"')

    def test_a_link_to_the_site_root_keeps_its_trailing_slash(self):
        self.assertEqual(h.relocate('href="../.."', "testing/coverage/", "testing/coverage-report/"), 'href="../../"')

    def test_the_page_linking_itself_is_rebased_as_a_directory(self):
        self.assertEqual(h.relocate('href="./"', "testing/coverage/", "testing/coverage-report/"), 'href="../coverage/"')

    def test_file_links_as_the_offline_bundle_writes_them(self):
        self.assertEqual(h.relocate('href="../support.html"', "testing/", "testing/coverage-report/Core/Source/"), 'href="../../../../support.html"')

    def test_external_anchor_and_data_urls_are_left_alone(self):
        for url in ("https://www.cososo.co.uk", "mailto:a@b", "#top", "data:image/svg+xml,x", "/absolute"):
            self.assertEqual(h.relocate(f'href="{url}"', "testing/coverage/", "x/"), f'href="{url}"')

    def test_a_query_or_fragment_follows_the_rebased_path(self):
        self.assertEqual(h.relocate('href="../../ci/#jobs"', "testing/coverage/", "testing/coverage-report/"), 'href="../../ci/#jobs"')


class Dressing(unittest.TestCase):
    def setUp(self):
        self.chrome = h.Chrome(TEMPLATE, "testing/coverage/", "testing/coverage/", CREDIT)
        self.out = self.chrome.dress(REPORT_PAGE, "testing/coverage-report/Platform/Posix/Source/")

    def test_the_sites_stylesheets_are_linked_before_the_override_sheet(self):
        main = self.out.index("../../../../../assets/stylesheets/main.css")
        self.assertLess(self.out.index("gcov.css"), main)
        self.assertLess(main, self.out.index("solidsyslog.css"))
        self.assertIn("https://fonts.googleapis.com/css?family=Open+Sans", self.out)

    def test_the_body_carries_the_sites_colour_scheme_attributes(self):
        self.assertIn('<body dir="ltr" data-md-color-scheme="default" data-md-color-primary="white">', self.out)
        self.assertNotIn("<body>", self.out)

    def test_the_header_replaces_the_simple_one(self):
        self.assertIn('<header class="md-header"', self.out)
        self.assertNotIn('<header class="ss-header">', self.out)
        self.assertIn('<img src="../../../../../assets/images/cososo-logo.svg" alt="logo">', self.out)
        self.assertIn('href="https://www.cososo.co.uk"', self.out)

    def test_the_header_loses_the_search_drawer_palette_and_script(self):
        for gone in ('class="md-search"', 'for="__drawer"', 'for="__search"', "md-header__option", "<script>"):
            self.assertNotIn(gone, self.out)
        self.assertIn("md-header__source", self.out)

    def test_the_title_line_links_back_to_the_introducing_page(self):
        self.assertIn('<a href="../../../../coverage/">Coverage</a>', self.out)

    def test_the_title_line_is_shown_without_the_sites_script(self):
        self.assertIn('class="md-header__title md-header__title--active"', self.out)

    def test_the_tabs_follow_the_header_with_their_links_rebased(self):
        self.assertIn('<a href="../../../../../" class="md-tabs__link">Home</a>', self.out)
        self.assertIn('<a href="../../../../coverage/" class="md-tabs__link">Testing</a>', self.out)

    def test_the_footer_names_the_reports_tool_and_keeps_the_build_line(self):
        self.assertIn(CREDIT, self.out)
        self.assertNotIn("Material for MkDocs", self.out)
        self.assertIn("Documentation built from main at 1234567", self.out)
        self.assertIn('<img src="../../../../../assets/images/solidsyslog-mark-white.svg">', self.out)
        self.assertNotIn('<footer class="ss-footer">', self.out)

    def test_the_sites_own_scripts_follow_the_footer_but_not_materials_bundle(self):
        self.assertIn('<script src="../../../../../javascripts/logo-scroll.js"></script>', self.out)
        self.assertNotIn("bundle.79ae519e.min.js", self.out)
        self.assertLess(self.out.index('<footer class="md-footer">'), self.out.index("logo-scroll.js"))

    def test_the_report_content_is_kept(self):
        self.assertIn("<table>coverage</table>", self.out)


class AtTheReportRoot(unittest.TestCase):
    def test_the_title_links_to_the_introducing_page_beside_the_report(self):
        chrome = h.Chrome(TEMPLATE, "testing/coverage/", "testing/coverage/", CREDIT)
        out = chrome.dress(REPORT_PAGE, "testing/coverage-report/")
        self.assertIn('<a href="../coverage/">Coverage</a>', out)


class OnASite(unittest.TestCase):
    def write(self, relative, text):
        path = os.path.join(self.site.name, relative)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(text)
        return path

    def setUp(self):
        self.site = tempfile.TemporaryDirectory()

    def tearDown(self):
        self.site.cleanup()

    def test_every_html_page_of_the_report_is_dressed(self):
        self.write("testing/coverage/index.html", TEMPLATE)
        top = self.write("testing/coverage-report/index.html", REPORT_PAGE)
        deep = self.write("testing/coverage-report/Core/Source/File.c.gcov.html", REPORT_PAGE)
        self.write("testing/coverage-report/gcov.css", "body {}")
        self.assertEqual(h.dress_report(self.site.name, "testing/coverage", "testing/coverage-report", CREDIT, True), 2)
        with open(deep, encoding="utf-8") as handle:
            self.assertIn('<a href="../../../coverage/">Coverage</a>', handle.read())
        with open(top, encoding="utf-8") as handle:
            self.assertIn('<a href="../coverage/">Coverage</a>', handle.read())

    def test_the_offline_bundles_page_layout_is_found(self):
        self.write("testing/coverage.html", TEMPLATE.replace("../../", "../"))
        page = self.write("testing/coverage-report/Core/Source/index.html", REPORT_PAGE)
        h.dress_report(self.site.name, "testing/coverage", "testing/coverage-report", CREDIT, False)
        with open(page, encoding="utf-8") as handle:
            self.assertIn('<img src="../../../../assets/images/cososo-logo.svg" alt="logo">', handle.read())

    def test_a_build_without_the_report_does_nothing(self):
        self.write("testing/coverage/index.html", TEMPLATE)
        self.assertEqual(h.dress_report(self.site.name, "testing/coverage", "testing/coverage-report", CREDIT, True), 0)

    def test_a_report_without_its_introducing_page_fails_the_build(self):
        self.write("testing/coverage-report/index.html", REPORT_PAGE)
        with self.assertRaises(h.PluginError):
            h.dress_report(self.site.name, "testing/coverage", "testing/coverage-report", CREDIT, True)


if __name__ == "__main__":
    unittest.main()

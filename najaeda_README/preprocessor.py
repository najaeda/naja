import argparse
import re
from pathlib import Path

class RSTSnippetInserter:
    def __init__(self, source_dir, source_rst, dest_rst):
        self.source_dir = Path(source_dir)
        self.source_rst = Path(source_rst)
        self.dest_rst = Path(dest_rst)
        self.snippet_pattern = re.compile(r"# snippet-start: (.+?)\n(.*?)# snippet-end: \1", re.DOTALL)
        self.placeholder_pattern = re.compile(r"\.\. snippet:: (.+)")

    def extract_snippets(self):
        snippets = {}
        for file in self.source_dir.rglob("*.py"):
            with file.open("r") as f:
                print(f"Extracting snippets from {file}")
                content = f.read()
                for match in self.snippet_pattern.finditer(content):
                    snippet_name = match.group(1)
                    snippet_code = match.group(2).strip()
                    print(f"Extracted snippet {snippet_name} from {file}")
                    snippets[snippet_name] = snippet_code
        return snippets

    def replace_placeholders(self, snippets):
        if not self.source_rst.exists():
            print(f"Source RST {self.source_rst} does not exist.")
            return

        target_rst = open(self.dest_rst, "w")

        content = self.source_rst.read_text()
        for match in self.placeholder_pattern.finditer(content):
            snippet_name = match.group(1)
            if snippet_name in snippets:
                code_block = f"\n.. code-block:: python\n\n    {snippets[snippet_name].replace('\n', '\n    ')}"
                content = content.replace(f".. snippet:: {snippet_name}", code_block)
            else:
                raise ValueError(f"Snippet {snippet_name} not found in source files. Available snippets: {snippets.keys()}")

        target_rst.write(content)
        print(f"Created {target_rst} with code snippets.")

    def run(self):
        snippets = self.extract_snippets()
        self.replace_placeholders(snippets)

def main():
    parser = argparse.ArgumentParser(description="Insert code snippets into RST files.")
    parser.add_argument(
        "--source_dir",
        required=True,
        help="Directory containing source files with snippets.",
    )
    parser.add_argument(
        "--source_rst",
        required=True,
        help="source RST file to update with snippets.",
    )
    parser.add_argument(
        "--dest_rst",
        required=True,
        help="dest RST file.",
    )
    args = parser.parse_args()

    inserter = RSTSnippetInserter(args.source_dir, args.source_rst, args.dest_rst)
    inserter.run()


if __name__ == "__main__":
    main()
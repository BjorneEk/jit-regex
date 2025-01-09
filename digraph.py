import sys
from graphviz import Source
from pathlib import Path

def main():
	# Check if the user provided a file argument
	if len(sys.argv) != 3:
		print("Usage: python digraph.py <graph_file.dot> <output>")
		sys.exit(1)

	# Get the graph file from the command-line arguments
	graph_file = sys.argv[1]
	out_file = sys.argv[2]
	try:
		# Read the DOT content from the file
		with open(graph_file, 'r') as f:
			dot_content = f.read()

		# Create a Graphviz Source object
		graph = Source(dot_content)
		output_file = Path(out_file).with_suffix(".pdf")
		# Render and display the graph
		graph.render(out_file, format=None, cleanup=True, view=True)
	except FileNotFoundError:
		print(f"Error: File '{graph_file}' not found.")
		sys.exit(1)
	except Exception as e:
		print(f"Error: {e}")
		sys.exit(1)

if __name__ == "__main__":
	main()
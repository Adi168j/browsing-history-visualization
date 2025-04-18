from flask import Flask, request, render_template, session, send_file
import os
import subprocess
import sqlite3
import pandas as pd
import logging
import time

# Setup
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

app = Flask(__name__)
app.secret_key = os.environ.get('SECRET_KEY', 'your_secret_key_here')

UPLOAD_FOLDER = 'uploads'
OUTPUT_FOLDER = 'outputs'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(OUTPUT_FOLDER, exist_ok=True)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['OUTPUT_FOLDER'] = OUTPUT_FOLDER

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/upload', methods=['POST'])
def upload_file():
    action = request.form.get('action')
    file = request.files.get('file')

    if file and file.filename != '':
        filename = file.filename
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        session['uploaded_file'] = filename
        session['csv_path'] = filepath
        logging.info(f"File uploaded to {filepath}")

        # Convert to CSV
        try:
            conn = sqlite3.connect(filepath)
            query = '''
                SELECT
                    urls.url,
                    urls.title,
                    urls.visit_count,
                    visits.visit_time as visit_time
                FROM urls, visits
                WHERE urls.id = visits.url
            '''
            df = pd.read_sql_query(query, conn)
            conn.close()

            csv_path = os.path.join(app.config['UPLOAD_FOLDER'], 'full_history.csv')
            df.to_csv(csv_path, index=False)
            session['csv_path'] = csv_path
        except Exception as e:
            return render_template('index.html', output=f"SQLite read error: {str(e)}", action=action)

    elif 'csv_path' in session:
        csv_path = session['csv_path']
        logging.info(f"Using cached CSV: {csv_path}")
    else:
        return render_template('index.html', output="No file selected or cached", action=action)

    # Run analyzer
    output_folder = run_cpp_program(session['csv_path'])
    if output_folder is None:
        return render_template('index.html', output="Analyzer failed.", action=action)

    try:
        if action == 'header':
            return render_output(output_folder, 'header.txt', action)
        elif action == 'summary':
            return render_output(output_folder, 'summary.txt', action)
        elif action == 'dfs':
            return render_output(output_folder, 'dfs_output.txt', action)
        elif action == 'graph':
            return render_output(output_folder, 'graph.txt', action)
        elif action == 'hops':
            return render_output(output_folder, 'shortest_path.txt', action)
        elif action == 'visits':
            return render_csv_table(output_folder, 'visit_counts.csv', 'visit_data', 'count', action)
        elif action == 'transitions':
            return render_csv_table(output_folder, 'transitions.csv', 'transitions_data', 'count', action)
        elif action == 'paths':
            return render_csv_table(output_folder, 'frequent_paths.csv', 'path_data', 'count', action)
        else:
            return render_template('index.html', output="Invalid action.", action=action)

    except Exception as e:
        return render_template('index.html', output=f"Error reading outputs: {str(e)}", action=action)

def run_cpp_program(csv_path):
    timestamp = int(time.time())
    folder = os.path.join(app.config['OUTPUT_FOLDER'], f"run_{timestamp}")
    os.makedirs(folder, exist_ok=True)

    # Use the exact same approach as before but with our updated analyzer
    command = ['./analyzer', csv_path, folder]

    try:
        logging.info(f"Running analyzer: {' '.join(command)}")
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
        
        if result.returncode == 0:
            logging.info(f"Analyzer completed successfully: {result.stdout}")
            return folder
        else:
            logging.error(f"Analyzer error: {result.stderr}")
            return None

    except subprocess.TimeoutExpired:
        logging.error("Analyzer timed out.")
        return None

    except Exception as e:
        logging.error(f"Analyzer failed: {str(e)}")
        return None

def read_file(path):
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    except Exception as e:
        logging.error(f"Failed to read file {path}: {str(e)}")
        return f"Failed to read file: {path}"

def render_output(folder, filename, action):
    path = os.path.join(folder, filename)
    if os.path.exists(path):
        content = read_file(path)
        return render_template('index.html', output=content, action=action)
    else:
        return render_template('index.html', output=f"{filename} not found in {folder}", action=action)

def render_csv_table(folder, filename, context_key, sort_col, action):
    path = os.path.join(folder, filename)
    if os.path.exists(path):
        try:
            data = pd.read_csv(path).nlargest(10, sort_col).to_dict(orient='records')
            return render_template('index.html', **{context_key: data}, action=action)
        except Exception as e:
            logging.error(f"Error processing CSV {path}: {str(e)}")
            return render_template('index.html', output=f"Error processing {filename}", action=action)
    else:
        return render_template('index.html', output=f"{filename} not found", action=action)

if __name__ == '__main__':
    app.run(debug=True)
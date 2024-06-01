import sqlite3
from flask import Flask, render_template, url_for

app = Flask(__name__)

@app.route('/')
def index():
    try:
        # Connect to the SQLite database
        conn = sqlite3.connect('ips.db')
        cursor = conn.cursor()

        # Execute a query to fetch all IP addresses
        cursor.execute("SELECT ip FROM ips")
        ips = cursor.fetchall()

        # Close the database connection
        conn.close()

        return render_template('index.html', ips=ips)
    except Exception as e:
        return f"An error occurred: {e}"

if __name__ == '__main__':
    app.run(debug=True)
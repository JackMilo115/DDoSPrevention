from flask import Flask, render_template
import sqlite3

app = Flask(__name__)

@app.route('/')
def index():
    try:
        conn = sqlite3.connect('ips.db')
        cursor = conn.cursor()
        cursor.execute("SELECT ip, count FROM ips ORDER BY count DESC LIMIT 5;")
        ips = cursor.fetchall()
        conn.close()

        return render_template('index.html', ips=ips)
    except Exception as e:
        return f"An error occurred: {e}"

if __name__ == '__main__':
    app.run(debug=True)
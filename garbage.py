with open('runtests.sh', 'r') as file:
    text = file.read()

text = text.replace('\r', '')

with open('runtests.sh', 'w') as file:
    file.write(text)

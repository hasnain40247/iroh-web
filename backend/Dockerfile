FROM node:20-bookworm

RUN apt-get update && apt-get install -y default-jdk gcc make && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY package*.json ./
RUN npm install --omit=dev

COPY server.js ./
COPY interpreter ./interpreter
COPY ciroh-src ./ciroh-src
COPY build-interpreters.sh ./

RUN bash build-interpreters.sh || echo "Interpreter build warnings (non-fatal)"

EXPOSE 3001
CMD ["node", "server.js"]

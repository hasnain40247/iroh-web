FROM eclipse-temurin:21-jdk-noble

# Install Node.js 20 and GCC
RUN apt-get update && apt-get install -y curl gcc make && \
    curl -fsSL https://deb.nodesource.com/setup_20.x | bash - && \
    apt-get install -y nodejs && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY package*.json ./
RUN npm install --omit=dev

COPY server.js ./
COPY interpreter ./interpreter
COPY ciroh-src ./ciroh-src
COPY build-interpreters.sh ./

RUN bash build-interpreters.sh || true

EXPOSE 3001
CMD ["node", "server.js"]
